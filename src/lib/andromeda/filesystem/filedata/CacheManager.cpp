
#include <cassert>
#include <chrono>

#include "CacheManager.hpp"
#include "PageManager.hpp"

#include "andromeda/SharedMutex.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
CacheManager::CacheManager() : 
    mDebug("CacheManager",this),
    mBandwidth(mDebug)
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

    if (mCurrentMemory > mMemoryLimit || mCurrentDirty > mDirtyLimit)
    {
        MDBG_INFO("... memory limit! signal");
        mThreadCV.notify_one();
    }

    // check mSkipMemoryWait since cleanup could be waiting on our dataLock
    if (canWait && newSize > oldSize) while (mSkipMemoryWait != &pageMgr &&
        (mCurrentMemory > mMemoryLimit || (mCurrentDirty > mDirtyLimit && isDirty)))
    {
        MDBG_INFO("... waiting for memory");
        mMemoryCV.wait(lock);
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
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " pages:" << mPageItMap.size() << ", memory:" << mCurrentMemory; });

    if (mDebug.GetLevel() >= Debug::Level::INFO)
    {
        uint64_t total = 0; for (const PageInfo& pageInfo : mPageQueue) total += pageInfo.mPageSize;
        if (total != mCurrentMemory){ MDBG_ERROR(": BAD MEMORY TRACKING! " << total << " != " << mCurrentMemory); assert(false); }
    }
}

/*****************************************************/
void CacheManager::PrintDirtyStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " dirtyPages:" << mDirtyItMap.size() << ", dirtyMemory:" << mCurrentDirty; });

    if (mDebug.GetLevel() >= Debug::Level::INFO)
    {
        uint64_t total = 0; for (const PageInfo& pageInfo : mDirtyQueue) total += pageInfo.mPageSize;
        if (total != mCurrentDirty){ MDBG_ERROR(": BAD DIRTY TRACKING! " << total << " != " << mCurrentDirty); assert(false); }
    }
}

/*****************************************************/
void CacheManager::CleanupThread()
{
    MDBG_INFO("()");

    while (true)
    {
        // need to make sure pageManager stays in scope between mMutex release and evict/flush
        PageManager::ScopeLock evictLock, flushLock;
        std::unique_ptr<PageInfo> currentEvict, currentFlush;

        { // lock scope
            UniqueLock lock(mMutex);

            while (mRunCleanup && mCurrentDirty <= mDirtyLimit &&
                mCurrentMemory + mMemoryMargin <= mMemoryLimit)
            {
                MDBG_INFO("... waiting");
                mThreadCV.wait(lock);
            }
            if (!mRunCleanup) break; // stop loop

            MDBG_INFO("... DOING CLEANUP!");
            PrintStatus(__func__, lock);

            while (!currentEvict && mCurrentMemory + mMemoryMargin > mMemoryLimit)
            {
                PageInfo& pageInfo { mPageQueue.front() };
                evictLock = pageInfo.mPageMgr.TryGetScopeLock();

                if (!evictLock) RemovePage(*pageInfo.mPagePtr, lock); // being deleted
                else 
                { // let waiters on this file continue so we can lock
                    currentEvict = std::make_unique<PageInfo>(pageInfo); // copy
                    mSkipMemoryWait = &currentEvict->mPageMgr;
                    mMemoryCV.notify_all();
                }
            }
        }

        // don't hold the local lock the whole time we're doing evict/flush as it could deadlock... to get the 
        // dataLock we have to wait in the file's dataLock queue and threads in the queue might do InformPage()

        if (currentEvict)
        {
            MDBG_INFO("... evicting page");

            SharedLockW dataLock { currentEvict->mPageMgr.GetWriteLock() };
            mSkipMemoryWait = nullptr; // have the lock
            currentEvict->mPageMgr.EvictPage(currentEvict->mPageIndex, dataLock);

            UniqueLock lock(mMutex);
            PrintStatus(__func__, lock);

            currentEvict.reset();
            mMemoryCV.notify_all();
        }

        { // lock scope
            UniqueLock lock(mMutex);

            PrintDirtyStatus(__func__, lock);
            while (!currentFlush && mCurrentDirty > mDirtyLimit)
            {
                PageInfo& pageInfo { mDirtyQueue.front() };
                flushLock = pageInfo.mPageMgr.TryGetScopeLock();

                if (!flushLock) RemovePage(*pageInfo.mPagePtr, lock); // being deleted
                else 
                { // let waiters on this file continue so we can lock
                    currentFlush = std::make_unique<PageInfo>(pageInfo); // copy
                    mSkipMemoryWait = &currentFlush->mPageMgr;
                    mMemoryCV.notify_all();
                }
            }
        }

        if (currentFlush)
        {
            MDBG_INFO("... flushing page");

            SharedLockRP dataLock { currentFlush->mPageMgr.GetReadPriLock() };
            mSkipMemoryWait = nullptr; // have the lock
            
            std::chrono::steady_clock::time_point timeStart { std::chrono::steady_clock::now() };
            size_t written { currentFlush->mPageMgr.FlushPage(currentFlush->mPageIndex, dataLock) };
            mDirtyLimit = mBandwidth.UpdateBandwidth(written, std::chrono::steady_clock::now()-timeStart);

            UniqueLock lock(mMutex);
            PrintDirtyStatus(__func__, lock);

            currentFlush.reset();
            mMemoryCV.notify_all();
        }
    }

    MDBG_INFO("... exiting");
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
