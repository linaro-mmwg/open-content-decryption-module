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

#ifndef MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COM_MEDIAENGINE_RPC_RPC_CDM_MEDIAENGINE_HANDLER_H_
#define MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COM_MEDIAENGINE_RPC_RPC_CDM_MEDIAENGINE_HANDLER_H_

#include <rpc/rpc.h>
#include <string>

#include <open_cdm_common.h>
#include <shmemsem_helper.h>
#include <open_cdm_mediaengine_com.h>

#if OCDM_SDP_ANY
#include "socket_client_helper.h"
#endif

namespace media {

/**
 * MediaEngineSession serves as an interface between a platform's mediaengine
 * and the CDMi. It is established as soon as EME signals 'keyAdded'.
 * RPC and shared memory are used for communication.
 * Secure channel decryption is not included.
 */
class RpcCdmMediaengineHandler : public OpenCdmMediaengineCom {
 public:
  static RpcCdmMediaengineHandler& getInstance();

  bool CreateMediaEngineSession(char *session_id_val, uint32_t session_id_len,
                           uint8_t *auth_data_val, uint32_t auth_data_len);

  DecryptResponse Decrypt(const uint8_t *pbIv, uint32_t cbIv,
                                  const uint8_t *pbData, uint32_t cbData,
                                  uint32_t *pdwSubSampleMapping, uint32_t cdwSubSampleMapping,
                                  uint8_t *out, uint32_t &out_size) override;
  int ReleaseMem() override;
  //TODO (sph): make out const
  ~RpcCdmMediaengineHandler() override;

 private:
  MediaEngineSessionId sessionId;
  RpcCdmMediaengineHandler();
  RpcCdmMediaengineHandler(RpcCdmMediaengineHandler const&);
  void operator=(RpcCdmMediaengineHandler const&);

  CLIENT *rpcClient;

#if OCDM_SDP_ANY
  SocketClient mSocketClient;
#endif

  // shared memory
  shmem_info *shMemInfo;
  int idXchngShMem;
  int idXchngSem;
};

}  // namespace media

#endif  // MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COM_MEDIAENGINE_RPC_RPC_CDM_MEDIAENGINE_HANDLER_H_
