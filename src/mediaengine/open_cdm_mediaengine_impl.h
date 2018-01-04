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

#ifndef MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_MEDIAENGINE_OPEN_CDM_MEDIAENGINE_IMPL_H_
#define MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_MEDIAENGINE_OPEN_CDM_MEDIAENGINE_IMPL_H_

#ifdef WPE
#include "open_cdm_mediaengine.h"
#include "open_cdm_mediaengine_com.h"
#else	//chrome
#include "media/cdm/ppapi/external_open_cdm/src/mediaengine/open_cdm_mediaengine.h"
#endif
#include <rpc/rpc.h>
#include <string>
#ifdef WPE
#include "../com/common/shmemsem/shmemsem_helper.h"
#else	//chrome
#include "media/cdm/ppapi/external_open_cdm/src/com/common/shmemsem/shmemsem_helper.h"
#include "media/cdm/ppapi/external_open_cdm/src/com/mediaengine/rpc/rpc_cdm_mediaengine_handler.h"
#endif
namespace media {

/**
 * MediaEngineSession serves as an interface between a platform's mediaengine
 * and the CDMi. It is established as soon as EME signals 'keyAdded'.
 * RPC and shared memory are used for communication.
 * Secure channel decryption is not included.
 */
class OpenCdmMediaengineImpl : public OpenCdmMediaengine {
 public:
  OpenCdmMediaengineImpl(char *session_id_val, uint32_t session_id_len);
  OpenCdmMediaengineImpl(char *session_id_val, uint32_t session_id_len,
                         uint8_t *auth_data_val, uint32_t auth_data_len);

  // synchronous decryption
  DecryptResponse Decrypt(const uint8_t *pbIv, uint32_t cbIv,
                                  const uint8_t *pbData, uint32_t cbData,
                                  uint8_t *out, uint32_t &out_size) override;
  int ReleaseMem() override;
  ~OpenCdmMediaengineImpl() override;
 private:
#ifdef WPE
  OpenCdmMediaengineCom *media_engine_com_;
#else
  RpcCdmMediaengineHandler *media_engine_com_;
#endif
};

}  // namespace media

#endif  // MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_MEDIAENGINE_OPEN_CDM_MEDIAENGINE_IMPL_H_
