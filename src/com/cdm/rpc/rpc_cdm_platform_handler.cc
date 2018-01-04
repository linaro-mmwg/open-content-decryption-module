/*
 * Copyright 2014 Fraunhofer FOKUS
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pthread.h>
#include <sys/types.h>
#include <wchar.h>
#include <map>
#ifdef WPE
#include "rpc_cdm_platform_handler.h"
#include <cdm_logging.h>
#else	//chrome
#include "media/cdm/ppapi/external_open_cdm/src/com/cdm/rpc/rpc_cdm_platform_handler.h"
#include "media/cdm/ppapi/cdm_logging.h"
#endif
#include <rpc/pmap_clnt.h>

extern "C" {
#ifdef WPE
#include <opencdm_xdr.h>
#else	//chrome
#include "media/cdm/ppapi/external_open_cdm/src/com/common/rpc/opencdm_xdr.h"
#endif
}

typedef struct {
  int prognum;
  int sock;
} thread_parm_t;
// TODO(ska): rename thread_parm_t


namespace media {

unsigned long gettransient(int proto, int vers, int *sockp) {
  static int prognum = 0x40000000;
  int s, len, socktype;
  struct sockaddr_in addr;
  switch (proto) {
    case IPPROTO_UDP:
      socktype = SOCK_DGRAM;
      break;
    case IPPROTO_TCP:
      socktype = SOCK_STREAM;
      break;
    default:
      CDM_DLOG() << "unknown protocol type";
      return 0;
  }
  if (*sockp == RPC_ANYSOCK) {
    if ((s = socket(AF_INET, socktype, 0)) < 0) {
      perror("socket");
      return (0);
    }
    *sockp = s;
  } else {
    s = *sockp;
  }
  addr.sin_addr.s_addr = 0;
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  len = sizeof(addr);
  // may be already bound, so don't check for error
  bind(s, (const sockaddr*) &addr, len);
  if (getsockname(s, reinterpret_cast<sockaddr*>(&addr),
                  reinterpret_cast<socklen_t*>(&len)) < 0)  {
    perror("getsockname");
    return (0);
  }
  while (!pmap_set(prognum++, vers, proto, ntohs(addr.sin_port)))
    continue;
  return (prognum - 1);
}

static std::map<int, RpcCdmPlatformHandler *> rpc_cdm_platform_con_map;

struct RpcThreadCallParam {
  RpcCdmPlatformHandler *caller;
  thread_parm_t *thread_parm;
};

RpcCdmPlatformHandler::RpcCdmPlatformHandler(
    OpenCdmPlatformComCallbackReceiver *callback_receiver)
: callback_receiver_(callback_receiver) {
  CDM_DLOG() << "new RpcCdmPlatformHandler instance";

  com_state = UNINITIALIZED;

  rpc_server_host = "localhost";

  // prepare starting threaded RPC Server
  pthread_t thread1;
  thread_parm_t *parm = NULL;
  int rc = 0;

  int sock = RPC_ANYSOCK;
  int prognum = gettransient(IPPROTO_TCP, 1, &sock);
  rpc_prog = prognum;
  rpc_client = NULL;

  if (prognum == 0) {
    com_state = FAULTY;
    CDM_DLOG() << "new RpcCdmPlatformHandler instance creation failed.";
    return;
  }
  rpc_cdm_platform_con_map[prognum] = this;

  /* set up multiple parameters to pass to the thread */
  parm = reinterpret_cast<thread_parm_t*>(malloc(sizeof(thread_parm_t)));
  parm->sock = sock;
  parm->prognum = prognum;
  RpcThreadCallParam *call_params = new RpcThreadCallParam();
  call_params->caller = this;
  call_params->thread_parm = parm;
  rc = pthread_create(&thread1, NULL, RpcCdmPlatformHandler::DelegateRpcInit,
                      call_params);
  CDM_DLOG() << "thread created: " << rc;

  com_state = INITIALIZED;
  // TODO(sph): pthread_exit to terminate thread
}

void *RpcCdmPlatformHandler::DelegateRpcInit(void *call_params) {
  RpcThreadCallParam *call_param =
      reinterpret_cast<RpcThreadCallParam *>(call_params);
  // delegate call to caller instance
  return call_param->caller->RpcInitPrivate(call_param->thread_parm);
}


void RpcCdmPlatformHandler::OnMessage1SvcDelegate(rpc_cb_message *kmm, struct svc_req *rqstp, RpcCdmPlatformHandler *p_instance)
{
  CDM_DLOG() << "on_key_message_1_svc";
  p_instance->OnMessage1Svc(kmm, rqstp);
}

void RpcCdmPlatformHandler::OnKeyStatusUpdate1SvcDelegate(
    rpc_cb_key_status_update *kmm, struct svc_req *rqstp,
    RpcCdmPlatformHandler *p_instance)
{
  CDM_DLOG() << "on_key_status_update_1_svc";
  OpenCdmPlatformSessionId session_id;
  std::string message = std::string(kmm->message);

  session_id.session_id_len = kmm->session_id.session_id_len;
  session_id.session_id = kmm->session_id.session_id_val;

#ifdef WPE
  if (strlen(kmm->message) > 0)
      message.assign("KeyUsable");
#endif

  CDM_DLOG() << "on_key_status_update_1_svc 1";
  p_instance->callback_receiver_->OnKeyStatusUpdateCallback(session_id, message);
  CDM_DLOG() << "on_key_status_update_1_svc 2";
}


void RpcCdmPlatformHandler::OnMessage1Svc(rpc_cb_message *kmm, struct svc_req *)
{
 std::string laURL(kmm->destination_url);
  std::string message(kmm->message);
  OpenCdmPlatformSessionId session_id;

  session_id.session_id_len = kmm->session_id.session_id_len;
  session_id.session_id = kmm->session_id.session_id_val;

 this->callback_receiver_->MessageCallback(session_id, message, laURL);
}

void RpcCdmPlatformHandler::OnReady1SvcDelegate(rpc_cb_ready *keyready_param, struct svc_req *rqstp, RpcCdmPlatformHandler *p_instance)
{
  CDM_DLOG() << "on_key_ready_1_svc";
  p_instance->OnReady1Svc(keyready_param, rqstp);
}

void RpcCdmPlatformHandler::OnReady1Svc(rpc_cb_ready *kr, struct svc_req *)
{
  CDM_DLOG() << "on_key_ready_1_svc";
  OpenCdmPlatformSessionId session_id;

  session_id.session_id_len = kr->session_id.session_id_len;
  session_id.session_id = kr->session_id.session_id_val;
  this->callback_receiver_->ReadyCallback(session_id);

}

void RpcCdmPlatformHandler::OnError1SvcDelegate(rpc_cb_error *err_param, struct svc_req *rqstp, RpcCdmPlatformHandler *p_instance)
{
  CDM_DLOG() << "on_key_error_1_svc";
  p_instance->OnError1Svc(err_param, rqstp);
}

void RpcCdmPlatformHandler::OnError1Svc(rpc_cb_error * ke, struct svc_req *)
{
  CDM_DLOG() << "on_key_error_1_svc";
  OpenCdmPlatformSessionId session_id;

  session_id.session_id_len = ke->session_id.session_id_len;
  session_id.session_id = ke->session_id.session_id_val;
  int sys_error = 0;
  // TODO (sph): set real error message if there is any
  this->callback_receiver_->ErrorCallback(session_id, sys_error,"KEY_ERROR");
}

//static void RpcCdmPlatformHandler::open_cdm_callback_1(struct svc_req *rqstp, register SVCXPRT *transp)
void RpcCdmPlatformHandler::RpcCallbackPrivate(struct svc_req *rqstp, register SVCXPRT *transp)
{
  union {
    rpc_cb_message on_message_1_arg;
    rpc_cb_ready on_ready_1_arg;
    rpc_cb_error on_error_1_arg;
  } argument;
  char *result;
  xdrproc_t _xdr_argument, _xdr_result;
  char *(*local)(char *, struct svc_req *, RpcCdmPlatformHandler *);

  CDM_DLOG() << "RpcCallbackPrivate \n";
  switch (rqstp->rq_proc) {
  case NULLPROC:
    (void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
    return;

  case ON_KEY_MESSAGE:
    _xdr_argument = (xdrproc_t) xdr_rpc_cb_message;
    _xdr_result = (xdrproc_t) xdr_void;
    local = (char *(*)(char *, struct svc_req *, RpcCdmPlatformHandler *)) RpcCdmPlatformHandler::OnMessage1SvcDelegate;
    break;

  case ON_KEY_READY:
    _xdr_argument = (xdrproc_t) xdr_rpc_cb_ready;
    _xdr_result = (xdrproc_t) xdr_void;
    local = (char *(*)(char *, struct svc_req *, RpcCdmPlatformHandler *)) RpcCdmPlatformHandler::OnReady1SvcDelegate;
    break;

  case ON_KEY_ERROR:
    _xdr_argument = (xdrproc_t) xdr_rpc_cb_error;
    _xdr_result = (xdrproc_t) xdr_void;
    local = (char *(*)(char *, struct svc_req *, RpcCdmPlatformHandler *)) RpcCdmPlatformHandler::OnError1SvcDelegate;
    break;

  case ON_KEY_STATUS_UPDATE:
    _xdr_argument = (xdrproc_t) xdr_rpc_cb_key_status_update;
    _xdr_result = (xdrproc_t) xdr_void;
    local = (char *(*)(char *, struct svc_req *, RpcCdmPlatformHandler *))
          RpcCdmPlatformHandler::OnKeyStatusUpdate1SvcDelegate;
    break;

  default:
    svcerr_noproc (transp);
    return;
  }
  memset ((char *)&argument, 0, sizeof (argument));
  if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
    svcerr_decode (transp);
    return;
  }
  result = (*local)((char *)&argument, rqstp, this);
  if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
    svcerr_systemerr (transp);
  }
  if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
    fprintf (stderr, "%s", "unable to free arguments");
    exit (1);
  }
  return;
}


void *RpcCdmPlatformHandler::RpcInitPrivate(void *thread_parm) {
  // async callback stuff

  SVCXPRT *xprt;
  thread_parm_t *p = reinterpret_cast<thread_parm_t*>(thread_parm);
  int sock = p->sock;
  int prognum = p->prognum;

  if ((xprt = svctcp_create(sock, 0, 0)) == NULL) {
    return NULL;
  }

  // protocol is 0 - gettransient does registering
  (void) svc_register(xprt, prognum, 1,
                      RpcCdmPlatformHandler::DelegateRpcCallback, IPPROTO_TCP);

  rpc_prog = prognum;

  svc_run();
  CDM_DLOG() << "svc_run executed";
  free(p);
  return NULL;
}

void RpcCdmPlatformHandler::DelegateRpcCallback(struct svc_req *rqstp,
                                                register SVCXPRT *transp) {
  if (rpc_cdm_platform_con_map.find(rqstp->rq_prog)
      != rpc_cdm_platform_con_map.end()) {
    RpcCdmPlatformHandler *caller = rpc_cdm_platform_con_map[rqstp->rq_prog];
    caller->RpcCallbackPrivate(rqstp, transp);
  }
}

MediaKeysResponse RpcCdmPlatformHandler::MediaKeys(std::string key_system) {
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeys";
  MediaKeysResponse response;

  // rpc not ready
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeys connection state faulty";
    return response;
  }

  if ((rpc_client = clnt_create(rpc_server_host.c_str(), OPEN_CDM,
                                OPEN_CDM_EME_5,
                                "tcp")) == NULL) {
    com_state = FAULTY;
    clnt_pcreateerror(rpc_server_host.c_str());
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler connection to server failed";
    return response;
  } else {
    CDM_DLOG() << "RpcCdmPlatformHandler connected to server";
  }
  // Cdm_MediaKeys
  rpc_response_generic *rpc_response;
  rpc_request_mediakeys rpc_param;
  rpc_param.key_system.key_system_val = reinterpret_cast<char *>(
      malloc(key_system.size()));
  memcpy(rpc_param.key_system.key_system_val, key_system.c_str(),
         key_system.size());
  rpc_param.key_system.key_system_len = key_system.size();
  if ((rpc_response = rpc_open_cdm_mediakeys_1(&rpc_param, rpc_client))
      == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }

  if (rpc_response->platform_val == 0) {
    CDM_DLOG() << "cdm_mediakeys_rpc_1 success\n ";
    response.platform_response = PLATFORM_CALL_SUCCESS;
  } else {
    CDM_DLOG() << "cdm_mediakeys_rpc_1 failed\n ";
    response.platform_response = PLATFORM_CALL_FAIL;
  }
  free(rpc_param.key_system.key_system_val);
  return response;
}
  //EME equivalent : media_key_.isTypeSupported()
MediaKeyTypeResponse RpcCdmPlatformHandler::IsTypeSupported(const std::string& key_system,
                                            const std::string& mime_type) {
  CDM_DLOG() << "RpcCdmPlatformHandler:: IsTypeSupported";
  MediaKeyTypeResponse response;

  // rpc not ready
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeys connection state faulty";
    return response;
  }

  if ((rpc_client = clnt_create(rpc_server_host.c_str(), OPEN_CDM,
                                OPEN_CDM_EME_5,
                                "tcp")) == NULL) {
    com_state = FAULTY;
    clnt_pcreateerror(rpc_server_host.c_str());
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler connection to server failed";
    return response;
  } else {
    CDM_DLOG() << "RpcCdmPlatformHandler connected to server";
  }

  //Pass keysystem
  rpc_response_generic *rpc_response;
  rpc_request_is_type_supported rpc_param;
  rpc_param.key_system.key_system_val = reinterpret_cast<char *>(
      malloc(key_system.size()));
  memcpy(rpc_param.key_system.key_system_val, key_system.c_str(),
         key_system.size());
  CDM_DLOG() << "Pass keysystem " << key_system << "\n";
  CDM_DLOG() << "Pass keysystem len  " << key_system.size();
  rpc_param.key_system.key_system_len = key_system.size();
 //pass mimeType
  rpc_param.mime_type.mime_type_val = reinterpret_cast<char *>(
      malloc(mime_type.size()));
  memcpy(rpc_param.mime_type.mime_type_val, mime_type.c_str(),
         mime_type.size());
  rpc_param.mime_type.mime_type_len = mime_type.size();

 //rpc call to server
  if ((rpc_response = rpc_open_cdm_is_type_supported_1(&rpc_param, rpc_client))
      == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }

  if (rpc_response->platform_val == 1) {
    CDM_DLOG() << "rpc_open_cdm_is_type_supported_1 success\n ";
    response.platform_response = PLATFORM_CALL_SUCCESS;
  } else {
    CDM_DLOG() << "rpc_open_cdm_is_type_supported_1 failed\n ";
    response.platform_response = PLATFORM_CALL_FAIL;
  }
  free(rpc_param.mime_type.mime_type_val);
  free(rpc_param.key_system.key_system_val);
  return response;
}

MediaKeySetServerCertificateResponse RpcCdmPlatformHandler::MediaKeySetServerCertificate(
  const uint8_t *pbServerCert, uint32_t cbServerCert) {
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeySetServerCertificate";
  fflush(stdout);
  MediaKeySetServerCertificateResponse response;
  rpc_response_generic *rpc_response;
  rpc_request_certificate rpc_param;
  // rpc not ready
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG()
    << "RpcCdmPlatformHandler::MediaKeySetServerCertificate connection state faulty";
    return response;
  }
  rpc_param.certificate.certificate_val = reinterpret_cast<uint8_t *>(malloc(cbServerCert));
  memcpy(rpc_param.certificate.certificate_val, pbServerCert, cbServerCert);
  rpc_param.certificate.certificate_len = cbServerCert;
  if ((rpc_response = rpc_open_cdm_mediakeys_set_server_certificate_1(
      &rpc_param, rpc_client)) == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }
  if (rpc_response) {
    if (rpc_response->platform_val == 0) {
       CDM_DLOG() << "MediaKeySetServerCertificate success\n ";
       response.platform_response = PLATFORM_CALL_SUCCESS;
    } else {
       CDM_DLOG() << "MediaKeySetServerCertificate failed\n ";
       response.platform_response = PLATFORM_CALL_FAIL;
    }
  }
  free(rpc_param.certificate.certificate_val);
  return response;
}

MediaKeysCreateSessionResponse RpcCdmPlatformHandler::MediaKeysCreateSession(
    const std::string& init_data_type, const uint8_t* init_data,
    int init_data_length)
{
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeysCreateSession";
  MediaKeysCreateSessionResponse response;

  // rpc not ready
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG()
    << "RpcCdmPlatformHandler::MediaKeysCreateSession connection state faulty";
    return response;
  }
  rpc_response_create_session *rpc_response;
  rpc_request_create_session rpc_param;
  rpc_param.init_data_type.init_data_type_val = reinterpret_cast<char *>(malloc(
      init_data_type.size()));
  memcpy(rpc_param.init_data_type.init_data_type_val, init_data_type.c_str(),
         init_data_type.size());
  rpc_param.init_data_type.init_data_type_len = init_data_type.size();

  rpc_param.init_data.init_data_val = reinterpret_cast<uint8_t *>(
      malloc(init_data_length));
  memcpy(rpc_param.init_data.init_data_val, init_data, init_data_length);
  rpc_param.init_data.init_data_len = init_data_length;

  std::string hostname = "localhost";
  // TODO(ska): specify dynamically, encapsulate RPC
  rpc_param.callback_info.hostname.hostname_val = reinterpret_cast<char *>(
      malloc(hostname.size()));
  memcpy(rpc_param.callback_info.hostname.hostname_val, hostname.c_str(),
         hostname.size());
  rpc_param.callback_info.hostname.hostname_len = hostname.size();
  rpc_param.callback_info.prog_num = rpc_prog;
  rpc_param.callback_info.prog_version = 1;
  // TODO(ska): specify dynamically, encapsulate RPC

  CDM_DLOG() << "createsession_rpc_1 ";

  if ((rpc_response = rpc_open_cdm_mediakeys_create_session_1(&rpc_param,
                                                              rpc_client))
      == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
    CDM_DLOG() << "error createsession_rpc_1";
  }

  // TODO(ska): parse session_id from csresult into
  OpenCdmPlatformSessionId session_id;
  if (rpc_response->platform_val == 0) {
    CDM_DLOG() << "MediaKeys_CreateSession success\n ";
    CDM_DLOG() << "CreateSession sid.len: " <<rpc_response->session_id.session_id_len;
    CDM_DLOG() << "CreateSession sid[0]: " << rpc_response->session_id.session_id_val[0];
    CDM_DLOG() << "CreateSession sid[1]: " << rpc_response->session_id.session_id_val[1];
    response.sys_err = rpc_response->platform_val;
    response.platform_response = PLATFORM_CALL_SUCCESS;
    session_id.session_id = rpc_response->session_id.session_id_val;
    session_id.session_id_len = rpc_response->session_id.session_id_len;
    response.session_id = session_id;

    response.licence_req  = std::string(rpc_response->licence_req.licence_req_val,
        rpc_response->licence_req.licence_req_len);
  } else {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "MediaKeys_CreateSession failed\n ";
  }
  free(rpc_param.callback_info.hostname.hostname_val);
  free(rpc_param.init_data.init_data_val);
  free(rpc_param.init_data_type.init_data_type_val);
  return response;
}

MediaKeysLoadSessionResponse RpcCdmPlatformHandler::MediaKeysLoadSession(
    char *session_id_val, uint32_t session_id_len) {
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeysLoadSession";
  MediaKeysLoadSessionResponse response;

  rpc_response_generic *rpc_response;
  rpc_request_load_session rpc_param;

  // rpc not ready
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG()
    << "RpcCdmPlatformHandler::MediaKeysLoadSession connection state faulty";
    return response;
  }

  rpc_param.session_id.session_id_val = session_id_val;
  rpc_param.session_id.session_id_len = session_id_len;

  if ((rpc_response = rpc_open_cdm_mediakeys_load_session_1(
      &rpc_param, rpc_client)) == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }

  if (rpc_response->platform_val == 0) {
    CDM_DLOG() << "MediaKeysLoadSession success\n ";
    response.platform_response = PLATFORM_CALL_SUCCESS;
  } else {
    CDM_DLOG() << "MediaKeysLoadSession failed\n ";
    response.platform_response = PLATFORM_CALL_FAIL;
  }

  return response;
}

MediaKeySessionUpdateResponse RpcCdmPlatformHandler::MediaKeySessionUpdate(
    const uint8 *pbKey, uint32 cbKey, char *session_id_val,
    uint32_t session_id_len)
{
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeySessionUpdate";
  MediaKeySessionUpdateResponse response;

  rpc_response_generic *rpc_response;
  rpc_request_session_update rpc_param;

  // rpc not ready
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG()
    << "RpcCdmPlatformHandler::MediaKeySessionUpdate connection state faulty";
    return response;
  }

  rpc_param.session_id.session_id_val = session_id_val;
  rpc_param.session_id.session_id_len = session_id_len;
  rpc_param.key.key_val = reinterpret_cast<uint8_t *>(malloc(cbKey));
  memcpy(rpc_param.key.key_val, pbKey, cbKey);
  rpc_param.key.key_len = cbKey;

  if ((rpc_response = rpc_open_cdm_mediakeysession_update_1(
      &rpc_param, rpc_client)) == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }
  if (rpc_response) {
    if (rpc_response->platform_val == 0) {
       CDM_DLOG() << "MediaKeySessionUpdate success\n ";
       response.platform_response = PLATFORM_CALL_SUCCESS;
    } else {
       CDM_DLOG() << "MediaKeySessionUpdate failed\n ";
       response.platform_response = PLATFORM_CALL_FAIL;
    }
  }
  free(rpc_param.key.key_val);
  return response;
}

MediaKeySessionRemoveResponse RpcCdmPlatformHandler::MediaKeySessionRemove(
    char *session_id_val, uint32_t session_id_len) {
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeySessionRemove";
  MediaKeySessionRemoveResponse response;

  rpc_response_generic *rpc_response;
  rpc_request_session_remove rpc_param;

  if (!rpc_client && ((rpc_client = clnt_create(rpc_server_host.c_str(), OPEN_CDM,
                                OPEN_CDM_EME_5,
                                "tcp")) == NULL)) {
    com_state = FAULTY;
    clnt_pcreateerror(rpc_server_host.c_str());
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler connection to server failed";
    return response;
  } else {
    CDM_DLOG() << "RpcCdmPlatformHandler connected to server";
  }

  rpc_param.session_id.session_id_val = session_id_val;
  rpc_param.session_id.session_id_len = session_id_len;

  if ((rpc_response = rpc_open_cdm_mediakeysession_remove_1(&rpc_param,
                                                             rpc_client))
      == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }

  if (rpc_response->platform_val == 0) {
    CDM_DLOG() << "MediaKeySessionRemove success\n ";
    response.platform_response = PLATFORM_CALL_SUCCESS;
  } else {
    CDM_DLOG() << "MediaKeySessionRemove failed\n ";
    response.platform_response = PLATFORM_CALL_FAIL;
  }

  return response;
}

MediaKeySessionCloseResponse RpcCdmPlatformHandler::MediaKeySessionClose(
    char *session_id_val, uint32_t session_id_len) {
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeySessionClose";
  MediaKeySessionCloseResponse response;

  rpc_response_generic *rpc_response;
  rpc_request_session_close rpc_param;

  if (!rpc_client && ((rpc_client = clnt_create(rpc_server_host.c_str(), OPEN_CDM,
                                OPEN_CDM_EME_5,
                                "tcp")) == NULL)) {
    com_state = FAULTY;
    clnt_pcreateerror(rpc_server_host.c_str());
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler connection to server failed";
    return response;
  } else {
    CDM_DLOG() << "RpcCdmPlatformHandler connected to server";
  }

  rpc_param.session_id.session_id_val = session_id_val;
  rpc_param.session_id.session_id_len = session_id_len;

  if ((rpc_response = rpc_open_cdm_mediakeysession_close_1(&rpc_param,
                                                             rpc_client))
      == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }

  if (rpc_response->platform_val == 0) {
    CDM_DLOG() << "MediaKeySessionClose success\n ";
    response.platform_response = PLATFORM_CALL_SUCCESS;
  } else {
    CDM_DLOG() << "MediaKeySessionClose failed\n ";
    response.platform_response = PLATFORM_CALL_FAIL;
  }

  return response;
}

MediaKeySessionReleaseResponse RpcCdmPlatformHandler::MediaKeySessionRelease(
    char *session_id_val, uint32_t session_id_len) {
  CDM_DLOG() << "RpcCdmPlatformHandler::MediaKeySessionRelease";
  MediaKeySessionReleaseResponse response;

  rpc_response_generic *rpc_response;
  rpc_request_session_release rpc_param;
#ifdef WPE
  if (!rpc_client && ((rpc_client = clnt_create(rpc_server_host.c_str(), OPEN_CDM,
                                OPEN_CDM_EME_5,
                                "tcp")) == NULL)) {
    com_state = FAULTY;
    clnt_pcreateerror(rpc_server_host.c_str());
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG() << "RpcCdmPlatformHandler connection to server failed";
    return response;
  } else {
    CDM_DLOG() << "RpcCdmPlatformHandler connected to server";
  }
#else	//chrome
  if (com_state == FAULTY) {
    response.platform_response = PLATFORM_CALL_FAIL;
    CDM_DLOG()
    << "RpcCdmPlatformHandler::MediaKeySessionRelease connection state faulty";
    return response;
  }
#endif

  rpc_param.session_id.session_id_val = session_id_val;
  rpc_param.session_id.session_id_len = session_id_len;

  if ((rpc_response = rpc_open_cdm_mediakeysession_release_1(&rpc_param,
                                                             rpc_client))
      == NULL) {
    clnt_perror(rpc_client, rpc_server_host.c_str());
  }

  if (rpc_response->platform_val == 0) {
    CDM_DLOG() << "MediaKeySessionRelease success\n ";
    response.platform_response = PLATFORM_CALL_SUCCESS;
  } else {
    CDM_DLOG() << "MediaKeySessionRelease failed\n ";
    response.platform_response = PLATFORM_CALL_FAIL;
  }

  return response;
}

}  // namespace media
