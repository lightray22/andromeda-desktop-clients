
#ifndef LIBA2_MEMORYALLOCATOR_H_
#define LIBA2_MEMORYALLOCATOR_H_

#include <unordered_map>
#include <mutex>
#include <string>

#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/**
 * A raw, non-caching memory allocator that allocates pages directly from the OS, bypassing the C library
 * NOTE memory is allocated only at page size granularity.  Use get_usage() to determine the actual memory size of an allocation.
 * In DEBUG builds, verifies all calls to free() for validity.
 * THREAD SAFE (INTERNAL LOCKS)
 */
class MemoryAllocator
{
public:

    explicit MemoryAllocator();

#if DEBUG
    virtual ~MemoryAllocator();
#endif // DEBUG

    /** Allocate the given number of bytes and return a pointer */
    void* alloc(size_t bytes);

    /**
     * Free a memory allocation returned by alloc().
     * @param bytes the number of bytes allocated (must match)
     */
    void free(void* const ptr, size_t bytes);

    /** 
     * Calculates the actual memory used by an allocation
     * Allocation sizes are always rounded by page granularity
     */
    size_t get_usage(const size_t bytes);

private:

    /** Updates and prints allocator statistics (debug) */
    void stats(const std::string& fname, const size_t bytes, bool alloc);

#if DEBUG
    typedef std::unordered_map<void*, size_t> AllocMap;
    /** Map of all allocations for verifying frees */
    AllocMap mAllocMap;
#endif // DEBUG

    /** total number of bytes allocated (debug) */
    size_t mTotal { 0 };
    /** total number of allocs (debug) */
    uint64_t mAllocs { 0 };
    /** total number of frees (debug) */
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
