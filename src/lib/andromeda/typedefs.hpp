
#ifndef LIBA2_TYPEDEFS_H_
#define LIBA2_TYPEDEFS_H_

#include <mutex>
#include <shared_mutex>

namespace Andromeda {

typedef std::unique_lock<std::mutex> UniqueLock;
typedef std::shared_lock<std::shared_mutex> SharedLockR;
typedef std::unique_lock<std::shared_mutex> SharedLockW;

} // namespace Andromeda

#endif // LIBA2_TYPEDEFS_H_
