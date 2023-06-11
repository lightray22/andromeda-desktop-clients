
#include "CachingAllocator.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
CachingAllocator::CachingAllocator(const size_t baseline) : 
    mDebug(__func__,this), mBaseline(baseline) { }

/*****************************************************/
CachingAllocator::~CachingAllocator()
{
    for (const FreeListMap::value_type& pair : mFreeLists)
        for (void* ptr : pair.second)
            MemoryAllocator::free(ptr, pair.first);
}

// FreeList: when freed, a page goes onto the front of the list for that alloc size
// the FreeList allows quick re-alloc by looking up the alloc size then taking the first entry (LIFO)
// FreeQueue: when freed, a page goes onto the front of the free queue
// the FreeQueue allows quick cleanup by popping a free off the end of the list (FIFO)

/*****************************************************/
void* CachingAllocator::alloc(size_t pages)
{
    MDBG_INFO("(pages:" << pages << " bytes:" << pages*mPageSize << ")");
    if (!pages) return nullptr;

    { // lock scope
        LockGuard lock(mMutex);
        ++mAllocs; // total
        mCurAlloc += pages*mPageSize;
        mMaxAlloc = std::max(mCurAlloc, mMaxAlloc);

        MDBG_INFO("... mBaseline:" << mBaseline << " mCurAlloc:" << mCurAlloc 
            << " mMaxAlloc:" << mMaxAlloc << " mMaxFree:" << mMaxAlloc-mBaseline);

        FreeListMap::iterator fmIt { mFreeLists.find(pages) };
        if (fmIt != mFreeLists.end())
        {
            FreeList& freeList { fmIt->second };

            void* ptr = freeList.front();
            freeList.pop_front();
            mFreeQueue.erase(ptr);
            mCurFree -= pages*mPageSize;
            ++mRecycles;

            // never have an empty list!
            if (freeList.empty())
                mFreeLists.erase(fmIt);

            MDBG_INFO("... recycle ptr:" << ptr 
                << " recycles:" << mRecycles << "/" << mAllocs);
            MDBG_INFO("... freeList:" << pages << ":" << freeList.size() 
                << " mCurFree:" << mCurFree);
            return ptr;
        }
    }

    void* ptr = MemoryAllocator::alloc(pages); // not locked!
    MDBG_INFO("... allocate ptr:" << ptr 
        << " recycles:" << mRecycles << "/" << mAllocs);
    return ptr;
}

/*****************************************************/
void CachingAllocator::free(void* const ptr, size_t pages) noexcept
{
    MDBG_INFO("(ptr:" << ptr << " pages:" << pages << " bytes:" << pages*mPageSize << ")");
    if (ptr == nullptr) return;

    LockGuard lock(mMutex);

    FreeListMap::iterator freeListMapIt { mFreeLists.emplace(pages, FreeList()).first }; // O(logn)
    FreeList& freeList { freeListMapIt->second };
    freeList.emplace_front(ptr); // O(1)
    mFreeQueue.enqueue_front(ptr, std::make_pair(freeListMapIt, freeList.begin())); // O(1)

    mCurFree += pages*mPageSize;
    mCurAlloc -= pages*mPageSize;

    MDBG_INFO("... freeList:" << pages << ":" << freeList.size() 
        << " freeQueue:" << mFreeQueue.size() << " mCurFree:" << mCurFree);

    while (mCurFree > mMaxAlloc-mBaseline) cleanup(lock);
}

/*****************************************************/
void CachingAllocator::cleanup(const LockGuard& lock)
{
    // free the oldest free
    FreeQueue::value_type fpair { mFreeQueue.pop_back() }; // O(1)

    void* ptr = fpair.first;
    FreeListMap::iterator& freeListMapIt { fpair.second.first };
    FreeList& freeList { freeListMapIt->second };
    const size_t pages { freeListMapIt->first };

    freeList.erase(fpair.second.second); // O(1)
    if (freeList.empty())
        mFreeLists.erase(freeListMapIt); // O(1)

    mCurFree -= pages*mPageSize;

    // free under lock to guarantee max memory
    MDBG_INFO("... free ptr:" << ptr);
    MemoryAllocator::free(ptr, pages);

    MDBG_INFO("... freeList:" << pages << ":" << freeList.size() 
        << " freeQueue:" << mFreeQueue.size() << " mCurFree:" << mCurFree);
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
