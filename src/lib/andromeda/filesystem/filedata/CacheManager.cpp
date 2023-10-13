
#include <cassert>
#include <chrono>

#include "CacheManager.hpp"
#include "CacheOptions.hpp"
#include "CachingAllocator.hpp"
#include "Page.hpp"
#include "PageManager.hpp"

#include "andromeda/StringUtil.hpp"
#include "andromeda/backend/BackendException.hpp"
using Andromeda::Backend::BackendException;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
CacheManager::CacheManager(const CacheOptions& cacheOptions, bool startThreads) : 
    mDebug(__func__,this),
    mCacheOptions(cacheOptions),
    mBandwidth(__func__, cacheOptions.maxDirtyTime)
{ 
    MDBG_INFO("()");

    const size_t memoryLimit { mCacheOptions.memoryLimit };
    const size_t allocBaseline { memoryLimit - memoryLimit/mCacheOptions.evictSizeFrac };
    mPageAllocator = std::make_unique<CachingAllocator>(allocBaseline);

    if (startThreads) StartThreads();
}

/*****************************************************/
size_t CacheManager::GetMemoryLimit() const
{ 
    return mCacheOptions.memoryLimit; 
}

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
void CacheManager::InformPage(PageManager& pageMgr, const uint64_t index, const Page& page, bool dirty, bool canWait, const SharedLockW* mgrLock)
{
    MDBG_INFO("(page:" << index << " " << &page << " canWait:" << BOOLSTR(canWait) << ")");

    UniqueLock lock(mMutex);

    const size_t oldSize { EnqueuePage(pageMgr, index, page, dirty, lock) };

    if (page.capacity() > oldSize)
    {
        HandleMemory(pageMgr, page, canWait, lock, mgrLock);
        if (dirty) HandleDirtyMemory(pageMgr, page, canWait, lock, mgrLock);
    }

    MDBG_INFO("... return!");
}

/*****************************************************/
size_t CacheManager::EnqueuePage(PageManager& pageMgr, const uint64_t index, const Page& page, bool dirty, const UniqueLock& lock) // cppcheck-suppress constParameter
{
    const size_t oldSize { RemovePage(page, lock) };
    const size_t newSize { page.capacity() }; // real memory usage

    const PageInfo pageInfo { pageMgr, index, newSize };

    mPageQueue.enqueue_front(&page, pageInfo);
    mCurrentTotal += newSize;

    PrintStatus(__func__, lock);

    if (dirty)
    {
        mDirtyQueue.enqueue_front(&page, pageInfo);
        mCurrentDirty += newSize;
        PrintDirtyStatus(__func__, lock);
    }

    MDBG_INFO("... pageSize:" << page.size() 
        << " newSize:" << newSize << " oldSize:" << oldSize);
    return oldSize;
}

/*****************************************************/
void CacheManager::ResizePage(const PageManager& pageMgr, const Page& page, const SharedLockW* mgrLock)
{
    const size_t newSize = page.capacity(); // real memory usage
    MDBG_INFO("... pageSize:" << page.size() << " newSize:" << newSize);

    UniqueLock lock(mMutex);

    { const PageQueue::iterator itQueue { mPageQueue.find(&page) };
    if (itQueue != mPageQueue.end()) 
    {
        const size_t oldSize { itQueue->second.mPageSize };
        mCurrentTotal += newSize-oldSize;
        itQueue->second.mPageSize = newSize;

        PrintStatus(__func__, lock);
        
        if (newSize > oldSize)
            HandleMemory(pageMgr, page, true, lock, mgrLock);
    } }

    { const PageQueue::iterator itQueue { mDirtyQueue.find(&page) };
    if (itQueue != mDirtyQueue.end()) 
    {
        const size_t oldSize { itQueue->second.mPageSize };
        mCurrentDirty += newSize-oldSize;
        itQueue->second.mPageSize = newSize;
        
        PrintDirtyStatus(__func__, lock);

        if (newSize > oldSize)
            HandleDirtyMemory(pageMgr, page, true, lock, mgrLock);
    } }
}

/*****************************************************/
void CacheManager::HandleMemory(const PageManager& pageMgr, const Page& page, bool canWait, UniqueLock& lock, const SharedLockW* mgrLock)
{
    MDBG_INFO("(canWait:" << BOOLSTR(canWait) << ")");

    if (mgrLock != nullptr && canWait)
    {
        // in this case we can evict synchronously rather than the background thread
        // so we can directly pick up errors (they could be missed due to mSkipEvictWait)
        while (ShouldEvict(lock) && 
            &mPageQueue.back().second.mPageMgr == &pageMgr &&
            mPageQueue.back().first != &page)
        {
            MDBG_INFO("... memory limit! synchronous evict");

            PrintStatus(__func__, lock);
            PageInfo pageInfo { mPageQueue.back().second }; // copy

            lock.unlock(); // don't hold lock during evict
            pageInfo.mPageMgr.EvictPage(pageInfo.mPageIndex, *mgrLock); // throws
            lock.lock();
        }
    }

    if (mEvictThread.joinable())
    {
        if (canWait)
        {
            mEvictFailure = nullptr; // reset error
            while (ShouldAwaitEvict(pageMgr, lock))
            {
                MDBG_INFO("... waiting for memory");
                mEvictThreadCV.notify_one();
                mEvictWaitCV.wait(lock);
            }
        }
        else if (ShouldEvict(lock))
        {
            MDBG_INFO("... memory limit! signal");
            mEvictThreadCV.notify_one();
        }
    }
}

/*****************************************************/
void CacheManager::HandleDirtyMemory(const PageManager& pageMgr, const Page& page, bool canWait, UniqueLock& lock, const SharedLockW* mgrLock)
{
    MDBG_INFO("(canWait:" << BOOLSTR(canWait) << ")");

    if (mgrLock != nullptr && canWait)
    {
        // in this case we can evict synchronously rather than the background thread
        // so we can directly pick up errors (they could be missed due to mSkipFlushWait)
        while (ShouldFlush(lock) && 
            &mDirtyQueue.back().second.mPageMgr == &pageMgr &&
            mDirtyQueue.back().first != &page)
        {
            MDBG_INFO("... dirty limit! synchronous flush");

            PrintDirtyStatus(__func__, lock);
            PageInfo pageInfo { mDirtyQueue.back().second }; // copy

            lock.unlock(); // don't hold lock during flush
            FlushPage(pageInfo.mPageMgr, pageInfo.mPageIndex, *mgrLock); // throws
            lock.lock();
        }
    }

    if (mFlushThread.joinable())
    {
        if (canWait)
        {
            mFlushFailure = nullptr; // reset error
            while (ShouldAwaitFlush(pageMgr, lock))
            {
                MDBG_INFO("... waiting for dirty memory");
                mFlushThreadCV.notify_one();
                mFlushWaitCV.wait(lock);
            }
        }
        else if (ShouldFlush(lock))
        {
            MDBG_INFO("... dirty limit! signal");
            mFlushThreadCV.notify_one();
        }
    }
}

/*****************************************************/
bool CacheManager::ShouldAwaitEvict(const PageManager& pageMgr, const UniqueLock& lock) const
{
    const bool shouldEvict { ShouldEvict(lock) };
    if (shouldEvict && mEvictFailure != nullptr)
        throw MemoryException("evict");

    // cleanup threads could be waiting on our mgrLock, see GetPageManagerLock
    return shouldEvict && !ShouldSkipWait(pageMgr, lock);
}

/*****************************************************/
bool CacheManager::ShouldAwaitFlush(const PageManager& pageMgr, const UniqueLock& lock) const
{
    const bool shouldFlush { ShouldFlush(lock) };
    if (shouldFlush && mFlushFailure != nullptr)
        throw MemoryException("flush");

    // cleanup threads could be waiting on our mgrLock, see GetPageManagerLock
    return shouldFlush && !ShouldSkipWait(pageMgr, lock);
}

/*****************************************************/
void CacheManager::RemovePage(const Page& page)
{
    const UniqueLock lock(mMutex);
    RemovePage(page, lock);

    PrintStatus(__func__, lock);
    PrintDirtyStatus(__func__, lock);
}

/*****************************************************/
void CacheManager::RemoveDirty(const Page& page)
{
    const UniqueLock lock(mMutex);
    RemoveDirty(page, lock);

    PrintDirtyStatus(__func__, lock);
}

/*****************************************************/
size_t CacheManager::RemovePage(const Page& page, const UniqueLock& lock)
{
    size_t pageSize { 0 }; // size of page removed
    const PageQueue::lookup_iterator itLookup { mPageQueue.lookup(&page) };
    if (itLookup != mPageQueue.lend())
    {
        MDBG_INFO("(page:" << &page << ")");
        pageSize = itLookup->second->second.mPageSize;
        mCurrentTotal -= pageSize;
        mPageQueue.erase(itLookup);
    }

    RemoveDirty(page, lock);
    return pageSize;
}

/*****************************************************/
void CacheManager::RemoveDirty(const Page& page, const UniqueLock& lock)
{
    const PageQueue::lookup_iterator itLookup { mDirtyQueue.lookup(&page) };
    if (itLookup != mDirtyQueue.lend())
    {
        MDBG_INFO("(page:" << &page << ")");
        mCurrentDirty -= itLookup->second->second.mPageSize;
        mDirtyQueue.erase(itLookup);
    }
}

/*****************************************************/
void CacheManager::PrintStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " pages:" << mPageQueue.size() << ", memory:" << mCurrentTotal; });

#if DEBUG // this will kill performance
    size_t total = 0; for (const PageQueue::value_type& pageInfo : mPageQueue) total += pageInfo.second.mPageSize;
    if (total != mCurrentTotal){ MDBG_ERROR(": BAD MEMORY TRACKING! " << total << " != " << mCurrentTotal); assert(false); }
#endif // DEBUG
}

/*****************************************************/
void CacheManager::PrintDirtyStatus(const char* const fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " dirtyPages:" << mDirtyQueue.size() << ", dirtyMemory:" << mCurrentDirty; });

#if DEBUG // this will kill performance
    size_t total = 0; for (const PageQueue::value_type& pageInfo : mDirtyQueue) total += pageInfo.second.mPageSize;
    if (total != mCurrentDirty){ MDBG_ERROR(": BAD DIRTY TRACKING! " << total << " != " << mCurrentDirty); assert(false); }
#endif // DEBUG
}

/*****************************************************/
SharedLockW CacheManager::GetPageManagerLock(PageManager& pageMgr, PageManager*& waitPtr)
{
    { // let waiters on this file continue so we can lock it
        const UniqueLock lock(mMutex);
        waitPtr = &pageMgr;

        // ShouldAwait() will check both to avoid deadlock
        mEvictWaitCV.notify_all();
        mFlushWaitCV.notify_all();
    }

    SharedLockW mgrLock { pageMgr.GetWriteLock() };
    
    { // have the mgrLock now
        const UniqueLock lock(mMutex);
        waitPtr = nullptr;
    }

    return mgrLock;
}

/*****************************************************/
void CacheManager::EvictThread()
{
    MDBG_INFO("()");

    while (true)
    {
        { // lock scope
            UniqueLock lock(mMutex);
            while (mRunCleanup && (mCurrentTotal <= mCacheOptions.memoryLimit || mEvictFailure != nullptr))
            {
                MDBG_INFO("... waiting");
                mEvictWaitCV.notify_all();
                mEvictThreadCV.wait(lock);
            }
            if (!mRunCleanup) break; // stop loop
            MDBG_INFO("... DOING EVICTS!");
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
                mFlushWaitCV.notify_all();
                mFlushThreadCV.wait(lock);
            }
            if (!mRunCleanup) break; // stop loop
            MDBG_INFO("... DOING FLUSHES!");
        }

        DoPageFlushes();
    }

    MDBG_INFO("... exiting");
}

/*****************************************************/
void CacheManager::DoPageEvictions() noexcept // thread cannot throw
{
    MDBG_INFO("()");

    // FIRST build a list of pages to evict
    PageMgrPageMap currentEvicts;
    { // lock scope
        const UniqueLock lock(mMutex);

        PrintStatus(__func__, lock);
        size_t cleaned { 0 };

        PageQueue::reverse_iterator pageIt { mPageQueue.rbegin() };
        const size_t margin { mCacheOptions.memoryLimit/mCacheOptions.evictSizeFrac };
        for (; mCurrentTotal + margin > mCacheOptions.memoryLimit + cleaned; ++pageIt)
        {
            const Page& pageRef { *pageIt->first };
            PageInfo& pageInfo { pageIt->second };

            const PageMgrPageMap::iterator evictIt { currentEvicts.find(&pageInfo.mPageMgr) };
            // get ScopeLock to make sure pageManager stays in scope between mMutex release and getting pageMgrW lock
            LockedPageList& evictSet { (evictIt != currentEvicts.end()) ? evictIt->second : currentEvicts.emplace(
                &pageInfo.mPageMgr, std::make_pair(pageInfo.mPageMgr.TryLockScope(), PageList())).first->second };

            if (!evictSet.first) // scope lock
                RemovePage(pageRef, lock); // being deleted
            else
            {
                evictSet.second.emplace_back(pageRef, pageInfo); // copy
                cleaned += pageInfo.mPageSize;
            }
        }
    }

    // THEN evict all the pages in the set
    for (PageMgrPageMap::iterator evictIt { currentEvicts.begin() }; evictIt != currentEvicts.end(); )
    {
        LockedPageList& evictSet { evictIt->second };
        MDBG_INFO("... evicting pages:" << evictSet.second.size() << " pageMgr:" << evictIt->first);
        const SharedLockW mgrLock { GetPageManagerLock(*evictIt->first, mSkipEvictWait) };

        for (const PageList::value_type& pagePair : evictSet.second)
        {
            const Page& pageRef { pagePair.first };
            const PageInfo& pageInfo { pagePair.second };

            try { pageInfo.mPageMgr.EvictPage(pageInfo.mPageIndex, mgrLock); }
            catch (const BackendException& ex)
            {
                const UniqueLock lock(mMutex);
                MDBG_ERROR("... " << ex.what());
                
                // move the failed page to the end so we try a different (maybe non-dirty) one next
                // evicting a non-dirty page doesn't use the backend so we can likely still evict those
                if (RemovePage(pageRef, lock)) // only if it's still enqueued (lock was released above)
                    EnqueuePage(pageInfo.mPageMgr, pageInfo.mPageIndex, pageRef, pageRef.isDirty(), lock);

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
void CacheManager::DoPageFlushes() noexcept // thread cannot throw
{
    MDBG_INFO("()");

    // FIRST build a list of pages to evict
    PageMgrPageMap currentFlushes;
    { // lock scope
        const UniqueLock lock(mMutex);

        PrintDirtyStatus(__func__, lock);
        size_t cleaned { 0 };

        PageQueue::reverse_iterator pageIt { mDirtyQueue.rbegin() };
        for (; mCurrentDirty > mDirtyLimit + cleaned; ++pageIt)
        {
            const Page& pageRef { *pageIt->first };
            PageInfo& pageInfo { pageIt->second };

            const PageMgrPageMap::iterator flushIt { currentFlushes.find(&pageInfo.mPageMgr) };
            // get ScopeLock to make sure pageManager stays in scope between mMutex release and getting pageMgrW lock
            LockedPageList& flushSet { (flushIt != currentFlushes.end()) ? flushIt->second : currentFlushes.emplace(
                &pageInfo.mPageMgr, std::make_pair(pageInfo.mPageMgr.TryLockScope(), PageList())).first->second };

            if (!flushSet.first) // scope lock
                RemovePage(pageRef, lock); // being deleted
            else
            {
                flushSet.second.emplace_back(pageRef, pageInfo); // copy
                cleaned += pageInfo.mPageSize;
            }
        }
    }

    // THEN flush all the pages in the set
    for (PageMgrPageMap::iterator flushIt { currentFlushes.begin() }; flushIt != currentFlushes.end(); )
    {
        LockedPageList& flushSet { flushIt->second };
        MDBG_INFO("... flushing pages:" << flushSet.second.size() << " pageMgr:" << flushIt->first);
        const SharedLockW mgrLock { GetPageManagerLock(*flushIt->first, mSkipFlushWait) };

        for (const PageList::value_type& pagePair : flushSet.second)
        {
            const PageInfo& pageInfo { pagePair.second };

            try { FlushPage(pageInfo.mPageMgr, pageInfo.mPageIndex, mgrLock); }
            catch (const BackendException& ex)
            {
                const UniqueLock lock(mMutex);
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
void CacheManager::FlushPage(PageManager& pageMgr, const uint64_t index, const SharedLockW& mgrLock)
{
    const std::chrono::steady_clock::time_point timeStart { std::chrono::steady_clock::now() };
    const size_t written { pageMgr.FlushPage(index, mgrLock) };

    const UniqueLock lock(mMutex); // protect mBandwidth/mDirtyLimit
    mDirtyLimit = mBandwidth.UpdateBandwidth(written, std::chrono::steady_clock::now()-timeStart);
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
