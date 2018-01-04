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

#ifndef MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_MEDIAENGINE_OPEN_CDM_MEDIAENGINE_COM_H_
#define MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_MEDIAENGINE_OPEN_CDM_MEDIAENGINE_COM_H_

#ifdef WPE
#include "open_cdm_mediaengine.h"
#else	//chrome
#include "media/cdm/ppapi/external_open_cdm/src/mediaengine/open_cdm_mediaengine.h"
#endif

namespace media {

  class OpenCdmMediaengineCom : public OpenCdmMediaengine {
   public:
    OpenCdmMediaengineCom(char *session_id_val, uint32_t session_id_len,
                          uint8_t *auth_data_val, uint32_t auth_data_len);
    ~OpenCdmMediaengineCom() override {}

   protected:
    OpenCdmMediaengineCom() {}
  };

}  // namespace media

#endif  // MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_MEDIAENGINE_OPEN_CDM_MEDIAENGINE_COM_H_
