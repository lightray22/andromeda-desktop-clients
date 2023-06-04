
#ifndef LIBA2_CACHINGALLOCATOR_H_
#define LIBA2_CACHINGALLOCATOR_H_

#include <list>
#include <map>
#include <mutex>

#include "MemoryAllocator.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

// TODO !! comments, yes thread safe
class CachingAllocator : public MemoryAllocator
{
public:

    explicit CachingAllocator();

    virtual ~CachingAllocator();

    void* alloc(size_t bytes);

    void free(void* const ptr, size_t bytes) noexcept;

private:

    typedef std::lock_guard<std::mutex> LockGuard;

    void cleanup(const LockGuard& lock);

    CachingAllocator(const CachingAllocator& a) = delete;
    CachingAllocator& operator=(const CachingAllocator&) = delete;

    Debug mDebug;
    std::mutex mMutex;

    uint64_t mMaxFree { 100*1024*1024 }; // TODO !! intelligent setting
    uint64_t mCurFree { 0 };

    uint64_t mRecycles { 0 };
    uint64_t mAllocs { 0 };

    typedef std::list<void*> PtrList;
    typedef std::map<size_t, PtrList> FreeMap;
    FreeMap mFreeMap;
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
        return reinterpret_cast<T* const>(mAlloc.alloc(n*sizeof(T)));
    }

    inline void deallocate(T* const p, const size_t b) noexcept
    {
        mAlloc.free(reinterpret_cast<void* const>(p), b);
    }
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHINGALLOCATOR_H_
