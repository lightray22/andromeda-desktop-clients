
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
    mDebug << __func__ << "()"; mDebug.Info();

    mThread = std::thread(&CacheManager::CleanupThread, this);
}

/*****************************************************/
CacheManager::~CacheManager()
{
    mDebug << __func__ << "()"; mDebug.Info();

    mRunCleanup = false;
    mThreadCV.notify_one();
    mThread.join();

    mDebug << __func__ << "... return"; mDebug.Info();
}

/*****************************************************/
void CacheManager::InformPage(PageManager& pageMgr, const uint64_t index, const Page& page)
{
    mDebug << __func__ << "(page:" << index << " " << &page << ")"; mDebug.Info();

    UniqueLock lock(mMutex);

    for (const PageMgrLists::value_type& pair : mCurrentEvicts)
        for (const PageInfo& pageInfo : pair.second)
            if (&pageMgr == &pageInfo.mPageMgr && index == pageInfo.mPageIndex)
            {
                mDebug << __func__ << "... already being evicted! return"; mDebug.Info();
                return; // do not re-add pages currently being evicted
            }

    ErasePage(page, lock);

    mPageQueue.emplace_back(PageInfo{pageMgr, index, &page, page.size()});
    mPageItMap[&page] = std::prev(mPageQueue.end());
    mCurrentMemory += page.size();

    PrintStatus(__func__, lock);

    if (mCurrentMemory > mMemoryLimit)
    {
        mDebug << __func__ << "... memory limit! signal"; mDebug.Info();
        if (!mCleanupActive) mThreadCV.notify_one();
    }
}

/*****************************************************/
void CacheManager::ResizePage(const Page& page, const size_t newSize)
{
    mDebug << __func__ << "(page:" << &page << ", newSize:" << newSize << ")"; mDebug.Info();

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
    else { mDebug << __func__ << "... page not found"; mDebug.Info(); }
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
    mDebug << __func__ << "(page:" << &page << ")"; mDebug.Info();

    PageItMap::iterator itLookup { mPageItMap.find(&page) };
    if (itLookup != mPageItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        mCurrentMemory -= itQueue->mPageSize;
        mPageItMap.erase(itLookup);
        mPageQueue.erase(itQueue);
    }
    else { mDebug << __func__ << "... page not found"; mDebug.Info(); }
}

/*****************************************************/
void CacheManager::PrintStatus(const std::string& func, const UniqueLock& lock)
{
    mDebug << func << "() pages:" << mPageItMap.size() 
        << ", memory:" << mCurrentMemory; mDebug.Info();
}

/*****************************************************/
void CacheManager::CleanupThread()
{
    mDebug << __func__ << "()"; mDebug.Info();

    while (mRunCleanup)
    {
        { // lock scope
            UniqueLock lock(mMutex);

            while (mRunCleanup && mCurrentMemory <= mMemoryLimit)
            {
                mDebug << __func__ << "... waiting"; mDebug.Info();
                mCleanupActive = false;
                mThreadCV.wait(lock);
            }
            if (!mRunCleanup) break;
            mCleanupActive = true;

            mDebug << __func__ << "... doing cleanup!"; mDebug.Info();

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

        mDebug << __func__ << "... numEvicts:" << mCurrentEvicts.size(); mDebug.Info();

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

    mDebug << __func__ << "... exiting"; mDebug.Info();
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
