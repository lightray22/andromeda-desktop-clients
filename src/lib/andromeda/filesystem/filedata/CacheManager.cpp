
#include <cassert>

#include "CacheManager.hpp"
#include "PageManager.hpp"

#include "andromeda/SharedMutex.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
CacheManager::CacheManager() : 
    mDebug("CacheManager",this) 
{ 
    MDBG_INFO("()");

    mThread = std::thread(&CacheManager::CleanupThread, this);
}

/*****************************************************/
CacheManager::~CacheManager()
{
    MDBG_INFO("()");

    mRunCleanup = false;
    mThreadCV.notify_one();
    mThread.join();

    MDBG_INFO("... return");
}

/*****************************************************/
void CacheManager::InformPage(PageManager& pageMgr, const uint64_t index, const Page& page, bool canWait)
{
    MDBG_INFO("(page:" << index << " " << &page << ")");

    UniqueLock lock(mMutex);

    const size_t oldSize { RemovePage(page, lock) };
    PageInfo pageInfo { pageMgr, index, &page, page.size() };

    mPageQueue.emplace_back(pageInfo);
    mPageItMap[&page] = std::prev(mPageQueue.end());
    mCurrentMemory += page.size();
    PrintStatus(__func__, lock);

    if (page.mDirty)
    {
        mDirtyQueue.emplace_back(pageInfo);
        mDirtyItMap[&page] = std::prev(mDirtyQueue.end());
        mCurrentDirty += page.size();
        PrintDirtyStatus(__func__, lock);
    }

    // copy in case the page is evicted while waiting
    const bool isDirty { page.mDirty };
    const size_t newSize { page.size() };

    // if we consumed more memory and cleanup is running, WAIT so memory doesn't keep increasing
    // BUT check currentEvict since it could be waiting on our W lock... skipping wait is fine in this
    // case since the cleanup's W lock will prevent further accesses to this file that could add memory

    if (mCurrentMemory > mMemoryLimit)
    {
        MDBG_INFO("... memory limit! signal");
        mThreadCV.notify_one();
    }

    if (canWait && newSize > oldSize)
        while (mCurrentMemory > mMemoryLimit && mCurrentEvict
                && &(mCurrentEvict->mPageMgr) != &pageMgr)
    {
        MDBG_INFO("... waiting for memory");
        mMemoryCV.wait(lock);
    }

    if (mCurrentDirty > mDirtyLimit)
    {
        MDBG_INFO("... dirty limit! signal");
        mThreadCV.notify_one();
    }

    if (canWait && newSize > oldSize && isDirty)
        while (mCurrentDirty > mDirtyLimit && mCurrentFlush
                && &(mCurrentFlush->mPageMgr) != &pageMgr)
    {
        MDBG_INFO("... waiting for dirty space");
        mDirtyCV.wait(lock);
    }

    MDBG_INFO("... return!");
}

/*****************************************************/
void CacheManager::ResizePage(const Page& page, const size_t newSize)
{
    MDBG_INFO("(page:" << &page << ", newSize:" << newSize << ")");

    UniqueLock lock(mMutex);

    { PageItMap::iterator itLookup { mPageItMap.find(&page) };
    if (itLookup != mPageItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        mCurrentMemory -= itQueue->mPageSize;
        itQueue->mPageSize = newSize;
        mCurrentMemory += newSize;

        PrintStatus(__func__, lock);
    }
    else { MDBG_INFO("... page not found"); } }

    { PageItMap::iterator itLookup { mDirtyItMap.find(&page) };
    if (itLookup != mDirtyItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        mCurrentDirty -= itQueue->mPageSize;
        itQueue->mPageSize = newSize;
        mCurrentDirty += newSize;

        PrintDirtyStatus(__func__, lock);
    }
    else { MDBG_INFO("... page not found"); } }
}

/*****************************************************/
void CacheManager::RemovePage(const Page& page)
{
    UniqueLock lock(mMutex);
    RemovePage(page, lock);

    PrintStatus(__func__, lock);
    PrintDirtyStatus(__func__, lock);
}

/*****************************************************/
void CacheManager::RemoveDirty(const Page& page)
{
    UniqueLock lock(mMutex);
    RemoveDirty(page, lock);

    PrintDirtyStatus(__func__, lock);
}

/*****************************************************/
size_t CacheManager::RemovePage(const Page& page, const UniqueLock& lock)
{
    MDBG_INFO("(page:" << &page << ")");

    size_t pageSize { 0 }; // size of page removed
    PageItMap::iterator itLookup { mPageItMap.find(&page) };
    if (itLookup != mPageItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        pageSize = itQueue->mPageSize;
        mCurrentMemory -= pageSize;
        mPageItMap.erase(itLookup);
        mPageQueue.erase(itQueue);
    }
    else { MDBG_INFO("... page not found"); }

    RemoveDirty(page, lock);
    return pageSize;
}

/*****************************************************/
void CacheManager::RemoveDirty(const Page& page, const UniqueLock& lock)
{
    MDBG_INFO("(page:" << &page << ")");

    PageItMap::iterator itLookup { mDirtyItMap.find(&page) };
    if (itLookup != mDirtyItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        mCurrentDirty -= itQueue->mPageSize;
        mDirtyItMap.erase(itLookup);
        mDirtyQueue.erase(itQueue);
    }
    else { MDBG_INFO("... page not found"); }
}

/*****************************************************/
void CacheManager::PrintStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "()"
        << " pages:" << mPageItMap.size() << ", memory:" << mCurrentMemory; });
}

/*****************************************************/
void CacheManager::PrintDirtyStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "()"
        << " dirtyPages:" << mDirtyItMap.size() << ", dirtyMemory:" << mCurrentDirty; });
}

/*****************************************************/
void CacheManager::CleanupThread()
{
    MDBG_INFO("()");

    while (true)
    {
        // need to make sure pageManager stays in scope between mMutex release and evict/flush
        PageManager::ScopeLock evictLock, flushLock;

        { // lock scope
            UniqueLock lock(mMutex);

            while (mRunCleanup && mCurrentDirty <= mDirtyLimit &&
                mCurrentMemory + mLimitMargin <= mMemoryLimit)
            {
                MDBG_INFO("... waiting");
                mThreadCV.wait(lock);
            }
            if (!mRunCleanup) break; // stop loop
            MDBG_INFO("... DOING CLEANUP!");

            PrintStatus(__func__, lock);
            while (!mCurrentEvict && mCurrentMemory + mLimitMargin > mMemoryLimit)
            {
                PageInfo& pageInfo { mPageQueue.front() };
                evictLock = pageInfo.mPageMgr.TryGetScopeLock();

                if (!evictLock) RemovePage(*pageInfo.mPagePtr, lock); // being deleted
                else mCurrentEvict = std::make_unique<PageInfo>(pageInfo); // copy
            }
        }

        // don't hold the local lock the whole time we're doing evict/flush as it could deadlock... to get the 
        // dataLock we have to wait in the file's dataLock queue and threads in the queue might do InformPage()

        if (mCurrentEvict)
        {
            MDBG_INFO("... evicting page");

            SharedLockW dataLock { mCurrentEvict->mPageMgr.GetWriteLock() };
            mCurrentEvict->mPageMgr.EvictPage(mCurrentEvict->mPageIndex, dataLock);

            UniqueLock lock(mMutex);
            PrintStatus(__func__, lock);

            mCurrentEvict.reset();
            mMemoryCV.notify_all();
        }

        { // lock scope
            UniqueLock lock(mMutex);

            PrintDirtyStatus(__func__, lock);
            while (!mCurrentFlush && mCurrentDirty > mDirtyLimit)
            {
                PageInfo& pageInfo { mDirtyQueue.front() };
                flushLock = pageInfo.mPageMgr.TryGetScopeLock();

                if (!flushLock) RemovePage(*pageInfo.mPagePtr, lock); // being deleted
                else mCurrentFlush = std::make_unique<PageInfo>(pageInfo); // copy
            }
        }

        if (mCurrentFlush)
        {
            MDBG_INFO("... flushing page");

            SharedLockR dataLock { mCurrentFlush->mPageMgr.GetReadLock() };
            mCurrentFlush->mPageMgr.FlushPage(mCurrentFlush->mPageIndex, dataLock);

            UniqueLock lock(mMutex);
            PrintDirtyStatus(__func__, lock);

            mCurrentFlush.reset();
            mDirtyCV.notify_all();
        }
    }

    MDBG_INFO("... exiting");
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
