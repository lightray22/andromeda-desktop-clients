
#ifndef LIBA2_CACHEMANAGER_H_
#define LIBA2_CACHEMANAGER_H_

#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/** Manages cache from a global perspective */
class CacheManager
{
public:
    CacheManager() = delete; // global static

};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHEMANAGER_H_
