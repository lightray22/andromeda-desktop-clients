
#ifndef LIBA2_MEMORYALLOCATOR_H_
#define LIBA2_MEMORYALLOCATOR_H_

#include <unordered_map>
#include <mutex>
#include <string>

#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

// TODO !! comments (only does PAGE_SIZE chunks (64K on windows), no caching), yes thread safe
class MemoryAllocator
{
public:

    explicit MemoryAllocator();

#if DEBUG
    virtual ~MemoryAllocator();
#endif // DEBUG

    void* alloc(size_t bytes);

    void free(void* const ptr, size_t bytes);

    /** 
     * Returns the actual memory used by an allocation
     * Allocation sizes are always rounded by page granularity
     */
    size_t get_usage(const size_t bytes);

private:

    void stats(const std::string& fname, const size_t bytes, bool alloc);

#if DEBUG
    typedef std::unordered_map<void*, size_t> AllocMap;
    /** Map of all allocations for verifying frees */
    AllocMap mAllocMap;
#endif // DEBUG

    /** total number of bytes allocated */
    uint64_t mTotal { 0 };
    /** total number of allocs */
    uint64_t mAllocs { 0 };
    /** total number of frees */
    uint64_t mFrees { 0 };
    /** stat-counter mutex */
    std::mutex mMutex;

    typedef std::lock_guard<decltype(mMutex)> LockGuard;

    Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_MEMORYALLOCATOR_H_
