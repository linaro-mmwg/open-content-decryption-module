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

#ifdef WPE
#include <shmemsem_helper.h>
#include <open_cdm_mediaengine_com.h>
#else	//chrome
#include "media/cdm/ppapi/external_open_cdm/src/com/common/shmemsem/shmemsem_helper.h"
#include "media/cdm/ppapi/external_open_cdm/src/mediaengine/open_cdm_mediaengine_com.h"
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
#ifdef WPE
  RpcCdmMediaengineHandler(char *session_id_val, uint32_t session_id_len,
                           uint8_t *auth_data_val, uint32_t auth_data_len);
#else	//chrome
  static RpcCdmMediaengineHandler& getInstance();

  bool CreateMediaEngineSession(char *session_id_val, uint32_t session_id_len,
                           uint8_t *auth_data_val, uint32_t auth_data_len);
#endif
  DecryptResponse Decrypt(const uint8_t *pbIv, uint32_t cbIv,
                                  const uint8_t *pbData, uint32_t cbData,
                                  uint8_t *out, uint32_t &out_size) override;
  int ReleaseMem() override;
  //TODO (sph): make out const
  ~RpcCdmMediaengineHandler() override;

 private:
  MediaEngineSessionId sessionId;
#ifndef WPE	//chrome
  RpcCdmMediaengineHandler();
  RpcCdmMediaengineHandler(RpcCdmMediaengineHandler const&);
  void operator=(RpcCdmMediaengineHandler const&);
#endif

  CLIENT *rpcClient;

  // shared memory
  shmem_info *shMemInfo;
  int idXchngShMem;
  int idXchngSem;
};

}  // namespace media

#endif  // MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COM_MEDIAENGINE_RPC_RPC_CDM_MEDIAENGINE_HANDLER_H_
