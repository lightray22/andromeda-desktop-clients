
#include "CachingAllocator.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
CachingAllocator::CachingAllocator() : mDebug(__func__,this){ }

/*****************************************************/
CachingAllocator::~CachingAllocator()
{
    for (FreeMap::value_type& pair : mFreeMap)
        for (void* ptr : pair.second)
            MemoryAllocator::free(ptr, pair.first);
}

// PtrList: when freed, a page goes onto the front of the list
// it can either get re-used in alloc (from the front - LIFO)
// or removed and deallocated in free (from the back - FIFO)

/*****************************************************/
void* CachingAllocator::alloc(size_t bytes)
{
    MDBG_INFO("(bytes:" << bytes << ")");
    if (!bytes) return nullptr;

    // our allocations will end up being rounded to page granularity anyway,
    // might as well be able to re-use allocations of the same actual size
    bytes = get_usage(bytes);

    { // lock scope
        LockGuard lock(mMutex);
        ++mAllocs; // total

        FreeMap::iterator fmIt { mFreeMap.find(bytes) };
        if (fmIt != mFreeMap.end())
        {
            PtrList& ptrList { fmIt->second };

            void* ptr = ptrList.front();
            ptrList.pop_front();
            mCurFree -= bytes;
            ++mRecycles;

            // never have an empty list!
            if (ptrList.empty())
                mFreeMap.erase(fmIt);

            MDBG_INFO("... recycle ptr:" << ptr 
                << " recycles:" << mRecycles << "/" << mAllocs);
            MDBG_INFO("... ptrList:" << bytes << ":" << ptrList.size() 
                << " mCurFree:" << mCurFree);
            return ptr;
        }
    }

    void* ptr = MemoryAllocator::alloc(bytes); // not locked!
    MDBG_INFO("... allocate ptr:" << ptr);
    return ptr;
}

/*****************************************************/
void CachingAllocator::free(void* const ptr, size_t bytes) noexcept
{
    MDBG_INFO("(ptr:" << ptr << " bytes:" << bytes << ")");
    if (ptr == nullptr) return;

    bytes = get_usage(bytes);

    { // lock scope
        LockGuard lock(mMutex);

        { // ptrList scope
            PtrList& ptrList { mFreeMap.emplace(bytes, PtrList()).first->second };

            ptrList.emplace_front(ptr);
            mCurFree += bytes;

            MDBG_INFO("... ptrList:" << bytes << ":" << ptrList.size() 
                << " mCurFree:" << mCurFree);
        }

        while (mCurFree > mMaxFree) cleanup(lock);
    }
}

/*****************************************************/
void CachingAllocator::cleanup(const LockGuard& lock)
{
    FreeMap::iterator fmIt { mFreeMap.begin() }; // free smallest allocs first

    const uint64_t bytes { fmIt->first };
    PtrList& ptrList { fmIt->second };

    void* ptr = ptrList.back();
    ptrList.pop_back();
    mCurFree -= get_usage(bytes);

    // never have an empty list!
    if (ptrList.empty())
        mFreeMap.erase(fmIt);

    // free under lock to guarantee max memory
    MDBG_INFO("... free ptr:" << ptr);
    MemoryAllocator::free(ptr, bytes);

    MDBG_INFO("... ptrList:" << bytes << ":" << ptrList.size() 
        << " mCurFree:" << mCurFree);
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
