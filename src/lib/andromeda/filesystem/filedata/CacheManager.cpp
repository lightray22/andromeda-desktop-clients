
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
void CacheManager::InformPage(PageManager& pageMgr, const uint64_t index, const Page& page)
{
    MDBG_INFO("(page:" << index << " " << &page << ")");

    UniqueLock lock(mMutex);

    for (const PageMgrLists::value_type& pair : mCurrentEvicts)
        for (const PageInfo& pageInfo : pair.second)
            if (&pageMgr == &pageInfo.mPageMgr && index == pageInfo.mPageIndex)
            {
                MDBG_INFO("... already being evicted! return");
                return; // do not re-add pages currently being evicted
            }

    ErasePage(page, lock);

    mPageQueue.emplace_back(PageInfo{pageMgr, index, &page, page.size()});
    mPageItMap[&page] = std::prev(mPageQueue.end());
    mCurrentMemory += page.size();

    PrintStatus(__func__, lock);

    if (mCurrentMemory > mMemoryLimit)
    {
        MDBG_INFO("... memory limit! signal");
        if (!mCleanupActive) mThreadCV.notify_one();
    }
}

/*****************************************************/
void CacheManager::ResizePage(const Page& page, const size_t newSize)
{
    MDBG_INFO("(page:" << &page << ", newSize:" << newSize << ")");

    UniqueLock lock(mMutex);

    PageItMap::iterator itLookup { mPageItMap.find(&page) };
    if (itLookup != mPageItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        mCurrentMemory -= itQueue->mPageSize;
        itQueue->mPageSize = newSize;
        mCurrentMemory += newSize;

        PrintStatus(__func__, lock);
    }
    else { MDBG_INFO("... page not found"); }
}

/*****************************************************/
void CacheManager::ErasePage(const Page& page)
{
    UniqueLock lock(mMutex);
    ErasePage(page, lock);

    PrintStatus(__func__, lock);
}

/*****************************************************/
void CacheManager::ErasePage(const Page& page, const UniqueLock& lock)
{
    MDBG_INFO("(page:" << &page << ")");

    PageItMap::iterator itLookup { mPageItMap.find(&page) };
    if (itLookup != mPageItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        mCurrentMemory -= itQueue->mPageSize;
        mPageItMap.erase(itLookup);
        mPageQueue.erase(itQueue);
    }
    else { MDBG_INFO("... page not found"); }
}

/*****************************************************/
void CacheManager::PrintStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << 
        "() pages:" << mPageItMap.size() << ", memory:" << mCurrentMemory; });
}

/*****************************************************/
void CacheManager::CleanupThread()
{
    MDBG_INFO("()");

    while (mRunCleanup)
    {
        { // lock scope
            UniqueLock lock(mMutex);

            while (mRunCleanup && mCurrentMemory <= mMemoryLimit)
            {
                MDBG_INFO("... waiting");
                mCleanupActive = false;
                mThreadCV.wait(lock);
            }
            if (!mRunCleanup) break;
            mCleanupActive = true;

            MDBG_INFO("... doing cleanup!");

            PrintStatus(__func__, lock);
            
            while (mCurrentMemory + mLimitMargin > mMemoryLimit)
            {
                const PageInfo& pageInfo { mPageQueue.front() };

                PageList& pageList { mCurrentEvicts.emplace(&pageInfo.mPageMgr, PageList()).first->second };
                pageList.push_back(pageInfo); // copy

                mCurrentMemory -= pageInfo.mPageSize;
                mPageItMap.erase(pageInfo.mPagePtr);
                mPageQueue.pop_front();
            }

            PrintStatus(__func__, lock);
        }

        // we cannot hold the local lock here while evicting a page as it can deadlock -
        // if we are evicting a page from a file then we need its dataLockW, but if it's
        // currently reading (has dataLockR) it will do InformPage() here and wait for our lock

        MDBG_INFO("... numEvicts:" << mCurrentEvicts.size());

        while (!mCurrentEvicts.empty())
        {
            PageMgrLists::iterator it { mCurrentEvicts.begin() };
            SharedLockW dataLock { it->first->GetWriteLock() };

            for (const PageInfo& pageInfo : it->second)
                it->first->EvictPage(pageInfo.mPageIndex, dataLock);

            UniqueLock lock(mMutex);
            it = mCurrentEvicts.erase(it);
        }
    }

    MDBG_INFO("... exiting");
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
