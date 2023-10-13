
#ifndef LIBA2_CACHINGALLOCATOR_H_
#define LIBA2_CACHINGALLOCATOR_H_

#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include <utility>

#include "MemoryAllocator.hpp"
#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/OrderedMap.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/**
 * Adds a very basic allocation-caching layer on top of MemoryAllocator.
 * When an allocation is free()d, it is added to a list to be re-used in a later alloc(). 
 * When the maximum free pool size is exceeded, allocations are removed and returned the OS smallest first.
 * NOTE 1) memory is allocated only at page size granularity.  Use get_usage() to determine the actual memory size of an allocation.
 * NOTE 2) allocations are only re-used if they are the exact size of a previously freed allocation.
 * ... these make this a bad general allocator, but good for allocating filedata pages that are mostly constant per-filesystem.
 * THREAD SAFE (INTERNAL LOCKS)
 */
class CachingAllocator : public MemoryAllocator
{
public:

    /** @param baseline the amount of memory used when evict stops, used to calculate the free pool max size */
    explicit CachingAllocator(size_t baseline);

    ~CachingAllocator() override;
    DELETE_COPY(CachingAllocator)
    DELETE_MOVE(CachingAllocator)

    /** 
     * Allocate the given number of pages and return a pointer
     * Returns a recycled (previously freed) pointer if possible
     */
    void* alloc(size_t pages) override;

    /**
     * Frees a range of pages allocated by alloc() - partial frees are allowed
     * Also adds the allocation to the free list and does cleanup.
     * @param ptr the pointer to free (must be aligned to a page boundary)
     * @param pages the number of pages to free
     */
    void free(void* ptr, size_t pages) override;

    struct Stats { 
        size_t curAlloc; size_t maxAlloc; size_t curFree; 
        size_t recycles; size_t allocs; };
    /** Returns a copy of some member variables for debugging */
    inline Stats GetStats() const { 
        const LockGuard lock(mMutex); return { 
            mCurAlloc, mMaxAlloc, mCurFree,
            mRecycles, mAllocs }; }

private:

    using LockGuard = std::lock_guard<std::mutex>;

    /** 
     * Adds an entry to the appropriate freeList and FreeQueue
     * @return the resulting size of the free list aded to
     */
    size_t add_entry(void* ptr, size_t pages, const LockGuard& lock) noexcept;

    /** Removes and returns to the OS the smallest freed allocation */
    void clean_entry(const LockGuard& lock);

    mutable Debug mDebug;
    mutable std::mutex mMutex;

    // the maximum size of the free pool is (mMaxAlloc-mBaseline)
    // ... the idea is that the Managers allocate new pages (reading, writing) BEFORE doing evictions,
    // so the memory usage can go beyond the cache max for small periods.  The peak size of this "overflow"
    // is the max size of our free pool.  This way the allocations will be available for the next read/write.

    /** The amount of memory used when evict stops */
    const size_t mBaseline;
    /** Current total memory allocated */
    size_t mCurAlloc { 0 };
    /** Peak total memory allocated */
    size_t mMaxAlloc { 0 };

    /** The current size (bytes) of the free pool */
    size_t mCurFree { 0 };

    /** The number of times an allocation was re-used (debug) */
    uint64_t mRecycles { 0 };
    /** The total number of calls to alloc() (debug) */
    uint64_t mAllocs { 0 };

    /** List of freed allocations that can be re-used */
    using FreeList = std::list<void*>;
    /** Map of freed pointer lists, indexed by allocation size for quick re-use */
    using FreeListMap = std::map<size_t, FreeList>;
    FreeListMap mFreeLists;

    /** Ordered map keeping track of freed allocations for LRU ordering */
    using FreeQueue = Andromeda::OrderedMap<void*, 
        std::pair<FreeListMap::iterator, FreeList::iterator>>;
    FreeQueue mFreeQueue;
};

/** C++ Allocator-compliant template wrapper for CachingAllocator */
template <typename T> struct CachingAllocatorT
{
    using value_type = T;

    CachingAllocator& mAlloc;
    /** Returns the inner non-template CachingAllocator */
    inline CachingAllocator& GetAllocator(){ return mAlloc; }

    explicit CachingAllocatorT(CachingAllocator& alloc): mAlloc(alloc) { };

    inline T* allocate(const size_t n)
    {
        const size_t pages { mAlloc.getNumPages(n*sizeof(T)) };
        return static_cast<T* const>(mAlloc.alloc(pages));
    }

    inline void deallocate(T* const p, const size_t b) noexcept
    {
        const size_t pages { mAlloc.getNumPages(b) };
        mAlloc.free(static_cast<void* const>(p), pages);
    }
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHINGALLOCATOR_H_
