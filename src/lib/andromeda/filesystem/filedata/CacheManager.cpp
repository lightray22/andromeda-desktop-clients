
#include <cassert>
#include <chrono>

#include "CacheManager.hpp"
#include "CacheOptions.hpp"
#include "PageManager.hpp"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
CacheManager::CacheManager(const CacheOptions& cacheOptions, bool startThreads) : 
    mCacheOptions(cacheOptions),
    mDebug("CacheManager",this),
    mBandwidth("CacheManager", cacheOptions.maxDirtyTime)
{ 
    MDBG_INFO("()");

    if (startThreads) StartThreads();
}

/*****************************************************/
uint64_t CacheManager::GetMemoryLimit() const { 
    return mCacheOptions.memoryLimit; }

/*****************************************************/
CacheManager::~CacheManager()
{
    MDBG_INFO("()");

    mRunCleanup = false;

    if (mEvictThread.joinable())
    {
        mEvictThreadCV.notify_one();
        mEvictThread.join();
    }

    if (mFlushThread.joinable())
    {
        mFlushThreadCV.notify_one();
        mFlushThread.join();
    }

    MDBG_INFO("... return");
}

/*****************************************************/
void CacheManager::StartThreads()
{
    MDBG_INFO("()");
    
    if (!mEvictThread.joinable()) // not running
        mEvictThread = std::thread(&CacheManager::EvictThread, this);

    if (!mFlushThread.joinable()) // not running
        mFlushThread = std::thread(&CacheManager::FlushThread, this);
}

/*****************************************************/
bool CacheManager::ShouldAwaitEvict(const PageManager& pageMgr, const UniqueLock& lock)
{
    if (mCurrentMemory > mCacheOptions.memoryLimit && mEvictFailure != nullptr)
        throw MemoryException("evict");

    // cleanup threads could be waiting on our mgrLock
    if (mSkipEvictWait == &pageMgr || mSkipFlushWait == &pageMgr) return false;

    return (mCurrentMemory > mCacheOptions.memoryLimit);
}

/*****************************************************/
bool CacheManager::ShouldAwaitFlush(const PageManager& pageMgr, const UniqueLock& lock)
{
    if (mCurrentDirty > mDirtyLimit && mFlushFailure != nullptr)
        throw MemoryException("flush");

    // cleanup threads could be waiting on our mgrLock
    if (mSkipEvictWait == &pageMgr || mSkipFlushWait == &pageMgr) return false;

    return (mCurrentDirty > mDirtyLimit);
}

/*****************************************************/
void CacheManager::HandleMemory(const PageManager& pageMgr, const Page& page, bool canWait, UniqueLock& lock, const SharedLockW* mgrLock)
{
    MDBG_INFO("(canWait:" << (canWait?"true":"false") << ")");

    if (mgrLock != nullptr)
    {
        // in this case we can evict synchronously rather than the background thread
        // so we can directly pick up errors (they could be missed due to mSkipEvictWait)
        while (canWait && mCurrentMemory > mCacheOptions.memoryLimit && 
            &mPageQueue.front().mPageMgr == &pageMgr &&
            mPageQueue.front().mPagePtr != &page)
        {
            MDBG_INFO("... memory limit! synchronous evict");

            PrintStatus(__func__, lock);
            PageInfo pageInfo { mPageQueue.front() }; // copy

            lock.unlock(); // don't hold lock during evict
            pageInfo.mPageMgr.EvictPage(pageInfo.mPageIndex, *mgrLock);
            lock.lock();
        }
    }

    if (mCurrentMemory > mCacheOptions.memoryLimit)
    {
        MDBG_INFO("... memory limit! signal");
        mEvictThreadCV.notify_one();
    }

    if (canWait && mEvictThread.joinable()) 
    {
        mEvictFailure = nullptr; // reset error
        while (ShouldAwaitEvict(pageMgr, lock))
        {
            MDBG_INFO("... waiting for memory");
            mEvictThreadCV.notify_one();
            mEvictWaitCV.wait(lock);
        }
    }
}

/*****************************************************/
void CacheManager::HandleDirtyMemory(const PageManager& pageMgr, const Page& page, bool canWait, UniqueLock& lock, const SharedLockW* mgrLock)
{
    MDBG_INFO("(canWait:" << (canWait?"true":"false") << ")");

    if (mgrLock != nullptr)
    {
        // in this case we can evict synchronously rather than the background thread
        // so we can directly pick up errors (they could be missed due to mSkipFlushWait)
        while (canWait && mCurrentDirty > mDirtyLimit && 
            &mDirtyQueue.front().mPageMgr == &pageMgr &&
            mDirtyQueue.front().mPagePtr != &page)
        {
            MDBG_INFO("... dirty limit! synchronous flush");

            PrintDirtyStatus(__func__, lock);
            PageInfo pageInfo { mDirtyQueue.front() }; // copy

            lock.unlock(); // don't hold lock during flush
            FlushPage(pageInfo.mPageMgr, pageInfo.mPageIndex, *mgrLock);
            lock.lock();
        }
    }

    if (mCurrentDirty > mDirtyLimit)
    {
        MDBG_INFO("... dirty limit! signal");
        mFlushThreadCV.notify_one();
    }

    if (canWait && mFlushThread.joinable())
    {
        mFlushFailure = nullptr; // reset error
        while (ShouldAwaitFlush(pageMgr, lock))
        {
            MDBG_INFO("... waiting for dirty memory");
            mFlushThreadCV.notify_one();
            mFlushWaitCV.wait(lock);
        }
    }
}

/*****************************************************/
size_t CacheManager::EnqueuePage(PageManager& pageMgr, const uint64_t index, const Page& page, const UniqueLock& lock)
{
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

    MDBG_INFO("... pageSize:" << page.size() << " oldSize:" << oldSize);
    return oldSize;
}

/*****************************************************/
void CacheManager::InformPage(PageManager& pageMgr, const uint64_t index, const Page& page, bool canWait, const SharedLockW* mgrLock)
{
    MDBG_INFO("(page:" << index << " " << &page << " canWait:" << (canWait?"true":"false") << ")");

    UniqueLock lock(mMutex);

    const size_t oldSize { EnqueuePage(pageMgr, index, page, lock) };
    if (page.size() > oldSize)
    {
        HandleMemory(pageMgr, page, canWait, lock, mgrLock);
        if (page.mDirty) 
            HandleDirtyMemory(pageMgr, page, canWait, lock, mgrLock);
    }

    MDBG_INFO("... return!");
}

/*****************************************************/
void CacheManager::ResizePage(const PageManager& pageMgr, const Page& page, const size_t newSize, const SharedLockW* mgrLock)
{
    MDBG_INFO("(page:" << &page << ", newSize:" << newSize << ")");

    UniqueLock lock(mMutex);

    { PageItMap::iterator itLookup { mPageItMap.find(&page) };
    if (itLookup != mPageItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        const size_t oldSize { itQueue->mPageSize };
        mCurrentMemory += newSize-oldSize;
        itQueue->mPageSize = newSize;

        PrintStatus(__func__, lock);

        if (newSize > oldSize)
            try { HandleMemory(pageMgr, page, true, lock, mgrLock); }
        catch (BaseException& e)
        {
            mCurrentMemory -= newSize-oldSize;
            itQueue->mPageSize = oldSize;
        }
    }
    else { MDBG_INFO("... page not found"); } }

    { PageItMap::iterator itLookup { mDirtyItMap.find(&page) };
    if (itLookup != mDirtyItMap.end()) 
    {
        PageList::iterator itQueue { itLookup->second };

        const size_t oldSize { itQueue->mPageSize };
        mCurrentDirty += newSize-oldSize;
        itQueue->mPageSize = newSize;
        
        PrintDirtyStatus(__func__, lock);

        if (newSize > oldSize)
            try { HandleDirtyMemory(pageMgr, page, true, lock, mgrLock); }
        catch (BaseException& e)
        {
            mCurrentDirty -= newSize-oldSize;
            itQueue->mPageSize = oldSize;
        }
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

#if DEBUG
    uint64_t total = 0; for (const PageInfo& pageInfo : mPageQueue) total += pageInfo.mPageSize;
    if (total != mCurrentMemory){ MDBG_ERROR(": BAD MEMORY TRACKING! " << total << " != " << mCurrentMemory); assert(false); }
#endif // DEBUG
}

/*****************************************************/
void CacheManager::PrintDirtyStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " dirtyPages:" << mDirtyItMap.size() << ", dirtyMemory:" << mCurrentDirty; });

#if DEBUG
    uint64_t total = 0; for (const PageInfo& pageInfo : mDirtyQueue) total += pageInfo.mPageSize;
    if (total != mCurrentDirty){ MDBG_ERROR(": BAD DIRTY TRACKING! " << total << " != " << mCurrentDirty); assert(false); }
#endif // DEBUG
}

/*****************************************************/
void CacheManager::EvictThread()
{
    MDBG_INFO("()");

    while (true)
    {
        { // lock scope
            UniqueLock lock(mMutex);
            while (mRunCleanup && (mCurrentMemory <= mCacheOptions.memoryLimit || mEvictFailure != nullptr))
            {
                MDBG_INFO("... waiting");
                mEvictThreadCV.wait(lock);
            }
            if (!mRunCleanup) break; // stop loop
            MDBG_INFO("... DOING CLEANUP!");
        }

        DoPageEvictions();
    }

    MDBG_INFO("... exiting");
}

/*****************************************************/
void CacheManager::FlushThread()
{
    MDBG_INFO("()");

    while (true)
    {
        { // lock scope
            UniqueLock lock(mMutex);
            while (mRunCleanup && (mCurrentDirty <= mDirtyLimit || mFlushFailure != nullptr))
            {
                MDBG_INFO("... waiting");
                mFlushThreadCV.wait(lock);
            }
            if (!mRunCleanup) break; // stop loop
            MDBG_INFO("... DOING CLEANUP!");
        }

        DoPageFlushes();
    }

    MDBG_INFO("... exiting");
}

/*****************************************************/
void CacheManager::DoPageEvictions()
{
    MDBG_INFO("()");

    // get ScopeLock to make sure pageManager stays in scope between mMutex release and getting pageMgrW lock
    typedef std::pair<PageManager::ScopeLock, PageList> EvictSet;
    std::map<PageManager*, EvictSet> currentEvicts;

    { // lock scope
        UniqueLock lock(mMutex);

        PrintStatus(__func__, lock);
        size_t cleaned { 0 };

        PageList::iterator pageIt { mPageQueue.begin() };
        const uint64_t margin { mCacheOptions.memoryLimit/mCacheOptions.evictSizeFrac };
        for (; mCurrentMemory + margin > mCacheOptions.memoryLimit + cleaned; ++pageIt)
        {
            PageInfo& pageInfo { *pageIt };

            decltype(currentEvicts)::iterator evictIt { currentEvicts.find(&pageInfo.mPageMgr) };
            EvictSet& evictSet { (evictIt != currentEvicts.end()) ? evictIt->second : currentEvicts.emplace(
                &pageInfo.mPageMgr, std::make_pair(pageInfo.mPageMgr.TryGetScopeLock(), PageList())).first->second };

            if (!evictSet.first) 
                RemovePage(*pageInfo.mPagePtr, lock); // being deleted
            else
            {
                evictSet.second.push_back(pageInfo); // copy
                cleaned += pageInfo.mPageSize;
            }
        }
    }

    decltype(currentEvicts)::iterator evictIt { currentEvicts.begin() };
    while (evictIt != currentEvicts.end())
    {
        MDBG_INFO("... evicting pages:" << evictIt->second.second.size() << " pageMgr:" << evictIt->first);

        mSkipEvictWait = evictIt->first; // let waiters on this file continue so we can lock
        mEvictWaitCV.notify_all();

        SharedLockW mgrLock { evictIt->first->GetWriteLock() };
        mSkipEvictWait = nullptr; // have the mgrLock now

        EvictSet& evictSet { evictIt->second };
        for (PageInfo& pageInfo : evictSet.second)
        {
            try { pageInfo.mPageMgr.EvictPage(pageInfo.mPageIndex, mgrLock); }
            catch (BaseException& ex)
            {
                UniqueLock lock(mMutex);
                MDBG_ERROR("... " << ex.what());
                
                // move the failed page to the front so we try a different (maybe non-dirty) one next
                if (RemovePage(*pageInfo.mPagePtr, lock))
                    EnqueuePage(pageInfo.mPageMgr, pageInfo.mPageIndex, *pageInfo.mPagePtr, lock);

                mEvictFailure = std::current_exception();
                mEvictWaitCV.notify_all();
                return; // early return
            }
        }

        evictIt = currentEvicts.erase(evictIt); // release scopeLock
        mEvictWaitCV.notify_all();
    }

    MDBG_INFO("... return!");
}

/*****************************************************/
void CacheManager::DoPageFlushes()
{
    MDBG_INFO("()");

    // get ScopeLock to make sure pageManager stays in scope between mMutex release and getting pageMgrW lock
    typedef std::pair<PageManager::ScopeLock, PageList> FlushSet;
    std::map<PageManager*, FlushSet> currentFlushes;

    { // lock scope
        UniqueLock lock(mMutex);

        PrintDirtyStatus(__func__, lock);
        size_t cleaned { 0 };

        PageList::iterator pageIt { mDirtyQueue.begin() };
        for (; mCurrentDirty > mDirtyLimit + cleaned; ++pageIt)
        {
            PageInfo& pageInfo { *pageIt };

            decltype(currentFlushes)::iterator flushIt { currentFlushes.find(&pageInfo.mPageMgr) };
            FlushSet& flushSet { (flushIt != currentFlushes.end()) ? flushIt->second : currentFlushes.emplace(
                &pageInfo.mPageMgr, std::make_pair(pageInfo.mPageMgr.TryGetScopeLock(), PageList())).first->second };

            if (!flushSet.first) 
                RemovePage(*pageInfo.mPagePtr, lock); // being deleted
            else
            {
                flushSet.second.push_back(pageInfo); // copy
                cleaned += pageInfo.mPageSize;
            }
        }
    }

    for (decltype(currentFlushes)::iterator flushIt { currentFlushes.begin() }; flushIt != currentFlushes.end(); )
    {
        MDBG_INFO("... flushing pages:" << flushIt->second.second.size() << " pageMgr:" << flushIt->first);

        mSkipFlushWait = flushIt->first; // let waiters on this file continue so we can lock
        mFlushWaitCV.notify_all();

        SharedLockRP mgrLock { flushIt->first->GetReadPriLock() };
        mSkipFlushWait = nullptr; // have the mgrLock now

        FlushSet& flushSet { flushIt->second };
        for (PageInfo& pageInfo : flushSet.second)
        {
            try { FlushPage(pageInfo.mPageMgr, pageInfo.mPageIndex, mgrLock); }
            catch (BaseException& ex)
            {
                UniqueLock lock(mMutex);
                MDBG_ERROR("... " << ex.what());
                mFlushFailure = std::current_exception();
                mFlushWaitCV.notify_all();
                return; // early return
            }
        }

        flushIt = currentFlushes.erase(flushIt); // release scopeLock
        mFlushWaitCV.notify_all();
    }

    MDBG_INFO("... return!");
}

/*****************************************************/
template<class T>
void CacheManager::FlushPage(PageManager& pageMgr, const uint64_t index, const T& mgrLock)
{
    std::chrono::steady_clock::time_point timeStart { std::chrono::steady_clock::now() };
    size_t written { pageMgr.FlushPage(index, mgrLock) };
    mDirtyLimit = mBandwidth.UpdateBandwidth(written, std::chrono::steady_clock::now()-timeStart);
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
