/*
 * Copyright 2016-2017 TATA ELXSI
 * Copyright 2016-2017 Metrological
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

#include "open_cdm.h"

#include <cdm_logging.h>
#include <cstdlib>
#include <open_cdm_common.h>
#include <open_cdm_mediaengine_factory.h>
#include <open_cdm_platform_factory.h>

using namespace std;

namespace media {

OpenCdm::OpenCdm()
    : media_engine_(NULL)
    , platform_(NULL) {
  CDM_DLOG() << "OpenDecryptor construct: key_system";
  platform_ = OpenCdmPlatformInterfaceFactory::Create(this);
  m_eState = KEY_SESSION_INIT;
}

OpenCdm::~OpenCdm() {
  CDM_DLOG() << "OpenCDM destruct";
  // clean up resources
  if (media_engine_) {
    delete(media_engine_);
  }

  if (platform_) {
    platform_->MediaKeySessionRelease(m_session_id.session_id, m_session_id.session_id_len);
    delete(platform_);
  }
}

void OpenCdm::SelectKeySystem(const std::string& key_system) {
  CDM_DLOG() << "OpenCdm: SelectKeySystem$" << "\n";
  m_key_system = key_system;
  platform_->MediaKeys(key_system);
  m_eState = KEY_SESSION_INIT;
}

void OpenCdm::SelectSession(const std::string& session_id_rcvd) {
  CDM_DLOG() << " Enter : SelectSession";
  m_session_id.session_id = strdup(session_id_rcvd.c_str());
  m_session_id.session_id_len = (uint32_t)session_id_rcvd.size();
}

int OpenCdm::SetServerCertificate(const uint8_t* server_certificate_data,
                                  uint32_t server_certificate_data_size) {
  MediaKeySetServerCertificateResponse ret = platform_->MediaKeySetServerCertificate((uint8_t*)server_certificate_data, server_certificate_data_size);
  if (ret.platform_response ==  PLATFORM_CALL_SUCCESS)
    return (true);
  else
    return (false);
}

int OpenCdm::CreateSession(const std::string& initDataType, unsigned char* pbInitData, int cbInitData, std::string& session_id, int licenseType) {
  int ret = 1;
  m_eState = KEY_SESSION_INIT;
  CDM_DLOG() << " Enter : CreateSession";
  MediaKeysCreateSessionResponse response = platform_->MediaKeysCreateSession(initDataType, pbInitData, cbInitData);
  CDM_DLOG() << "Contin : CreateSession ";
  if (response.platform_response == PLATFORM_CALL_SUCCESS) {
    CDM_DLOG() << "New Session created";
    m_session_id = response.session_id;
    if (KEY_SESSION_INIT == m_eState)
      m_eState = KEY_SESSION_WAITING_FOR_MESSAGE;
    ret = 0;
  } else {
        CDM_DLOG() << "FAIL to create session!";
        m_eState = KEY_SESSION_ERROR;
  }

  session_id.assign(m_session_id.session_id, m_session_id.session_id_len);// initializing out parameter.

  return ret;
}

// caller provided buffer, MAX_LENGTH...
int OpenCdm::GetKeyMessage(std::string& challenge, int* challengeLength,
    unsigned char* licenseURL, int* urlLength) {

  std::unique_lock<std::mutex> lck(m_mtx);
  CDM_DLOG() << "GetKeyMessage >> invoked from ocdm :: estate = " << m_eState << "\n";

  while (m_eState == KEY_SESSION_WAITING_FOR_MESSAGE) {
    CDM_DLOG() << "Waiting for key message!";
    m_cond_var.wait(lck);
  }

  CDM_DLOG() << "Key message should be ready or no need key challenge/message."<< "\n" ;

  if (m_eState == KEY_SESSION_MESSAGE_RECEIVED) {
    char temp[m_message.length()];

    m_message.copy(temp,m_message.length(),0);
    challenge.assign((const char*)temp, m_message.length());
    strncpy((char*)licenseURL, (const char*)m_dest_url.c_str(), m_dest_url.length());
    *challengeLength = m_message.length();
    *urlLength = m_dest_url.length();
    char msg[m_message.length()];
    m_message.copy( msg, m_message.length() , 0);
    CDM_DLOG() << "setting m_eState to KEY_SESSION_WAITING_FOR_LICENSE";
    m_eState = KEY_SESSION_WAITING_FOR_LICENSE;
  }

  if (m_eState == KEY_SESSION_READY) {
    *challengeLength = 0;
    *licenseURL= 0;
  }
  return 0;
}

int OpenCdm::Load(std::string& responseMsg) {
  int ret = 1;
  CDM_DLOG() << "Load >> invoked from ocdm :: estate = " << m_eState << "\n";
  CDM_DLOG() << "Load session with info exisiting key.";
  
  m_eState = KEY_SESSION_WAITING_FOR_LOAD_SESSION;
  MediaKeysLoadSessionResponse status = platform_->MediaKeysLoadSession(m_session_id.session_id, m_session_id.session_id_len);
  if (status.platform_response ==  PLATFORM_CALL_SUCCESS) {
    CDM_DLOG() << "Load session with info exisiting key complete.";

     while (m_eState == KEY_SESSION_WAITING_FOR_LOAD_SESSION) {
       CDM_DLOG() << "Waiting for load message!";
       std::unique_lock<std::mutex> lck(m_mtx);
       m_cond_var.wait(lck);
     }

     if (m_eState == KEY_SESSION_UPDATE_LICENSE || m_eState == KEY_SESSION_MESSAGE_RECEIVED) {
       while (m_eState == KEY_SESSION_LOADED) {
         CDM_DLOG() << "Waiting for Load message!";
         std::unique_lock<std::mutex> lck(m_mtx);
         m_cond_var.wait(lck);
       }
       if (m_eState == KEY_SESSION_UPDATE_LICENSE) {
         ret = 0;
         CDM_DLOG() << "setting m_eState to KEY_SESSION_LOADED";
         m_eState = KEY_SESSION_LOADED; //TODO : tobe rechecked and updated
       }
       if (m_eState == KEY_SESSION_MESSAGE_RECEIVED) {
         ret = 0;
         CDM_DLOG() << "setting m_eState to KEY_SESSION_EXPIRED";
         m_eState = KEY_SESSION_EXPIRED; //TODO : tobe rechecked and updated
         responseMsg.assign("message:");
      }
    }
  }
  responseMsg.append(m_message.c_str(), m_message.length());
  return ret;
}

int OpenCdm::Update(uint8_t* pbResponse, int cbResponse, std::string& responseMsg) {
  CDM_DLOG() << "Update >> invoked from ocdm :: estate = " << m_eState << "\n";

  int ret = 1;
  CDM_DLOG() << "Update session with info from server.";
  platform_->MediaKeySessionUpdate((uint8_t*)pbResponse, cbResponse, m_session_id.session_id, m_session_id.session_id_len);
  CDM_DLOG() << "Update session with info from server complete.";
  while (m_eState == KEY_SESSION_WAITING_FOR_LICENSE) {
    CDM_DLOG() << "Waiting for license update status!" << "\n";
    fflush(stdout);
    std::unique_lock<std::mutex> lck(m_mtx);
    m_cond_var.wait(lck);
  }
  if (m_eState == KEY_SESSION_UPDATE_LICENSE || m_eState == KEY_SESSION_REMOVED) {
    ret = 0;
  }
  else {
    CDM_DLOG() <<"Got license update status!" << "\n";
    fflush(stdout);
    if (m_eState == KEY_SESSION_MESSAGE_RECEIVED) {
      m_eState = KEY_SESSION_WAITING_FOR_LICENSE;
      responseMsg.assign("message:");
    }
  }
  responseMsg.append(m_message.c_str(), m_message.length());
  return ret;
}

int OpenCdm::Remove(std::string& responseMsg) {
  CDM_DLOG() << "Remove >> invoked from ocdm :: estate = " << m_eState << "\n";
  int ret = 1;
  CDM_DLOG() << "\nEnd";
  CDM_DLOG() << "Remove session with info exisiting key.";

  m_eState = KEY_SESSION_WAITING_FOR_LICENSE_REMOVAL;
  MediaKeySessionRemoveResponse status = platform_->MediaKeySessionRemove(m_session_id.session_id, m_session_id.session_id_len);
  if (status.platform_response ==  PLATFORM_CALL_SUCCESS) {
    CDM_DLOG() << "Remove session with info exisiting key complete.";
  
    while (m_eState == KEY_SESSION_WAITING_FOR_LICENSE_REMOVAL) {
      CDM_DLOG() << "Waiting for remove message!";
      std::unique_lock<std::mutex> lck(m_mtx);
      m_cond_var.wait(lck);
    }

    if (m_eState == KEY_SESSION_REMOVED || m_eState == KEY_SESSION_MESSAGE_RECEIVED) {
        while (m_eState == KEY_SESSION_REMOVED) {
           CDM_DLOG() << "Waiting for remove message!";
           std::unique_lock<std::mutex> lck(m_mtx);
           m_cond_var.wait(lck);
        }
        if (m_eState == KEY_SESSION_MESSAGE_RECEIVED) {
           ret = 0;
           CDM_DLOG() << "setting m_eState to KEY_SESSION_REMOVED";
           m_eState = KEY_SESSION_REMOVED;//TODO : tobe rechecked and updated
           responseMsg.assign("message:");
      }
   }
  }
  responseMsg.append(m_message.c_str(), m_message.length());
  return ret;
}

int OpenCdm::Close() {
  CDM_DLOG() << "Close >> invoked from ocdm :: estate = " << m_eState << "\n";
  CDM_DLOG() << "\nEnd";
  CDM_DLOG() << "Close session with info existing key.";
  MediaKeySessionCloseResponse status = platform_->MediaKeySessionClose(m_session_id.session_id, m_session_id.session_id_len);
  CDM_DLOG() << "Close session with info existing key complete.";
  if (status.platform_response ==  PLATFORM_CALL_SUCCESS) {
    m_eState = KEY_SESSION_CLOSED;
    return (true);
  }
  else
    return (false);
}

int OpenCdm::ReleaseMem() {
  if(media_engine_)
     return media_engine_->ReleaseMem();

  return (false);
}

int OpenCdm::Decrypt(unsigned char* encryptedData, uint32_t encryptedDataLength, unsigned char* ivData, uint32_t ivDataLength) {
  int ret = 1;
  uint32_t outSize;
  CDM_DLOG() << "OpenCdm::Decrypt session_id : " << m_session_id.session_id << endl;
  CDM_DLOG() << "OpenCdm::Decrypt session_id_len : " << m_session_id.session_id_len << endl;
  CDM_DLOG() << "OpenCdm::Decrypt encryptedDataLength : " << encryptedDataLength << endl;
  CDM_DLOG() << "OpenCdm::Decrypt ivDataLength : " << ivDataLength << endl;
  // mediaengine instantiation
  if (!media_engine_) {
    // FIXME:(ska): handle mutiple sessions
    media_engine_ = OpenCdmMediaengineFactory::Create(m_key_system, m_session_id);
    CDM_DLOG() << "::" << endl;
    if (!media_engine_){
      CDM_DLOG() << "::" << endl;
      return ret;
    }
  }

  CDM_DLOG() << "Returned back to OpenCdm::Decrypt";
  DecryptResponse dr = media_engine_->Decrypt((const uint8_t*)ivData, ivDataLength,
      (const uint8_t*)encryptedData, encryptedDataLength, (uint8_t*)encryptedData, outSize);

  CDM_DLOG() << "media_engine_->Decrypt done " << dr.platform_response;

  return 0;
}

bool OpenCdm::IsTypeSupported(const std::string& keySystem, const std::string& mimeType) {
  MediaKeyTypeResponse ret;

  ret = platform_->IsTypeSupported(keySystem, mimeType);
  CDM_DLOG() << "IsTypeSupported ";

  if (ret.platform_response ==  PLATFORM_CALL_SUCCESS )
    return (true);
  else
    return (false);
}

void OpenCdm::ReadyCallback(OpenCdmPlatformSessionId platform_session_id) {

  CDM_DLOG() << "OpenCdm::ReadyCallback";

  std::unique_lock<std::mutex> lck(m_mtx);
  m_eState = KEY_SESSION_READY;
  m_cond_var.notify_all();

  CDM_DLOG() << "OpenCdm::ReadyCallback";
}

void OpenCdm::ErrorCallback(OpenCdmPlatformSessionId platform_session_id,
    uint32_t sys_err, std::string err_msg) {
  CDM_DLOG() << "OpenCdm::ErrorCallback";
  std::unique_lock<std::mutex> lck(m_mtx);
  m_message = err_msg;
  m_eState = KEY_SESSION_ERROR;
  m_cond_var.notify_all();
}

void OpenCdm::MessageCallback(OpenCdmPlatformSessionId platform_session_id,
    std::string message, std::string destination_url) {

  CDM_DLOG() << "OpenCdm::MessageCallback:";
  fflush(stdout);
  CDM_DLOG() << "Start MessageCallback";
  for(int i = 0; i < message.length(); i++)
    printf("%2x ",message[i]);
  CDM_DLOG() << "End MessageCallback";
  std::unique_lock<std::mutex> lck(m_mtx);
  m_message = message;
  m_dest_url = destination_url;
  m_eState = KEY_SESSION_MESSAGE_RECEIVED;
  m_cond_var.notify_all();
}

void OpenCdm::OnKeyStatusUpdateCallback(OpenCdmPlatformSessionId platform_session_id, std::string message) {

  if (message == "KeyUsable")
    m_eState = KEY_SESSION_UPDATE_LICENSE;
  else if (message == "KeyReleased")
    m_eState = KEY_SESSION_REMOVED;
  else if (message == "KeyExpired")
    m_eState = KEY_SESSION_EXPIRED;
  else
    m_eState = KEY_SESSION_ERROR;

  m_message = message;
  m_cond_var.notify_all();

  CDM_DLOG() << "Got key status update ->  " << message;
}
} // namespace media
