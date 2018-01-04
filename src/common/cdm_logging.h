
#ifndef MEDIA_CDM__CDM_LOGGING_H_
#define MEDIA_CDM__CDM_LOGGING_H_

#include <iostream>
namespace media {

#define CDM_DLOG() std::cout << "\n" <<__FILE__<<":"<<  __func__ <<":"<< __LINE__ <<"::"
}  // namespace media

#endif  // MEDIA_CDM_CDM_LOGGING_H_
