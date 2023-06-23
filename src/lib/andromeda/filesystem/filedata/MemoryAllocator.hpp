
#ifndef LIBA2_MEMORYALLOCATOR_H_
#define LIBA2_MEMORYALLOCATOR_H_

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

#include "andromeda/Debug.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/**
 * A raw, non-caching memory allocator that allocates pages directly from the OS, bypassing the C library.
 * In DEBUG builds, verifies all calls to free() for validity.
 * THREAD SAFE (INTERNAL LOCKS)
 */
class MemoryAllocator
{
public:

    explicit MemoryAllocator();

#if DEBUG
    virtual ~MemoryAllocator();
#else // !DEBUG
    virtual ~MemoryAllocator() = default;
#endif // DEBUG
    DELETE_COPY(MemoryAllocator)
    DELETE_MOVE(MemoryAllocator)

    /** Allocate the given number of pages and return a pointer */
    virtual void* alloc(size_t pages);

    /**
     * Frees a range of pages allocated by alloc() - partial frees are allowed
     * @param ptr the pointer to free (must be aligned to a page boundary)
     * @param pages the number of pages to free
     */
    virtual void free(void* ptr, size_t pages);

    /** Returns the number of bytes in each page */
    [[nodiscard]] inline size_t getPageSize() const { return mPageSize; }

    /** Calculates the number of pages needed to hold the given number of bytes (page granularity) */
    [[nodiscard]] inline size_t getNumPages(const size_t bytes) const { 
        return bytes ? (bytes-1)/mPageSize+1 : 0; }

    /** Returns the actual number of bytes used for an allocation (page granularity) */
    [[nodiscard]] inline size_t getNumBytes(const size_t bytes) const {
        return getNumPages(bytes)*getPageSize(); }

protected:

    /** The minimum size of OS memory mappings */
    const size_t mPageSize;

private:

    /** Ask the OS for the page granularity */
    [[nodiscard]] size_t calcPageSize() const;

    /** Updates and prints allocator statistics (debug) */
    void stats(const std::string& fname, size_t pages, bool alloc);

#if DEBUG
    using AllocMap = std::map<void*, size_t, std::greater<>>;
    /** Map of all allocations for verifying frees */
    AllocMap mAllocMap;
#endif // DEBUG
    
    /** total number of pages allocated (debug) */
    size_t mTotalPages { 0 };
    /** total number of bytes allocated (debug) */
    size_t mTotalBytes { 0 };
    /** total number of allocs (debug) */
    uint64_t mAllocs { 0 };
    /** total number of frees (debug) */
    uint64_t mFrees { 0 };
    /** stat-counter mutex */
    std::mutex mMutex;

    using LockGuard = std::lock_guard<decltype(mMutex)>;
    mutable Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_MEMORYALLOCATOR_H_
