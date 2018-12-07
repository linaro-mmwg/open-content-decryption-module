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

#ifndef MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COMMON_OPEN_CDM_COMMON_H_
#define MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COMMON_OPEN_CDM_COMMON_H_

#ifdef OCDM_USE_PLAYREADY
#include "playready/constants.h"
#else
#ifdef CLEAR_KEY_CDM_USE_FFMPEG_DECODER
#include "clearkey/constants.h"
#endif
#endif


namespace media {
const char kOpenCdmVersion[] = "1.0.0.0";
}  // namespace media


/* OpenCDM Secure Data Path configuration */
/* SDP disabled: OpenCDM do not use SDP. When OCDM_SDP is not defined, SDP
   is disabled. */
#define OCDM_SDP_TYPE_DISABLED  0
/* SDP prototype: OpenCDM allocates a buffer from the ION heap specified by
   ION_SECURE_HEAP_ID_DECODER. It decrypts the data into that buffer, maps the
   buffer and copies the data into the destination buffer. This assumes that
   there is no memory protection on the ION heap. This is used to validate
   the ION integration in OpenCDM and DRM layers. */
#define OCDM_SDP_TYPE_PROTOTYPE 1
/* End-to-end SDP: OpenCDM gets a file descriptor referencing a secure ION
   buffer. It provides the file descriptor to the DRM layer where the data
   is decrypted into the secure memory. */
#define OCDM_SDP_TYPE_END2END   2

#define OCDM_SDP_PROTOTYPE (defined(OCDM_SDP) && (OCDM_SDP == OCDM_SDP_TYPE_PROTOTYPE))
#define OCDM_SDP_END2END   (defined(OCDM_SDP) && (OCDM_SDP == OCDM_SDP_TYPE_END2END))
#define OCDM_SDP_ANY       (OCDM_SDP_PROTOTYPE || OCDM_SDP_END2END)
#define OCDM_SDP_DISABLED  (!OCDM_SDP_ANY)


#endif  // MEDIA_CDM_PPAPI_EXTERNAL_OPEN_CDM_COMMON_OPEN_CDM_COMMON_H_
