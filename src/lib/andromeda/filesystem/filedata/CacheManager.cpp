
#include <cassert>
#include <chrono>

#include "CacheManager.hpp"
#include "CacheOptions.hpp"
#include "CachingAllocator.hpp"
#include "Page.hpp"
#include "PageManager.hpp"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;

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
    MDBG_INFO("(page:" << index << " " << &page << " canWait:" << (canWait?"true":"false") << ")");

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
size_t CacheManager::EnqueuePage(PageManager& pageMgr, const uint64_t index, const Page& page, bool dirty, const UniqueLock& lock)
{
    const size_t oldSize { RemovePage(page, lock) };
    const size_t newSize { page.capacity() }; // real memory usage

    const PageInfo pageInfo { pageMgr, index, newSize };

    mPageQueue.enqueue_front(&page, pageInfo);
    mCurrentMemory += newSize;

    static const std::string fname { __func__ };
    PrintStatus(fname, lock);

    if (dirty)
    {
        mDirtyQueue.enqueue_front(&page, pageInfo);
        mCurrentDirty += newSize;
        PrintDirtyStatus(fname, lock);
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
    static const std::string fname { __func__ };

    { PageQueue::iterator itQueue { mPageQueue.find(&page) };
    if (itQueue != mPageQueue.end()) 
    {
        const size_t oldSize { itQueue->second.mPageSize };
        mCurrentMemory += newSize-oldSize;
        itQueue->second.mPageSize = newSize;

        PrintStatus(fname, lock);
        
        if (newSize > oldSize)
            HandleMemory(pageMgr, page, true, lock, mgrLock);
    } }

    { PageQueue::iterator itQueue { mDirtyQueue.find(&page) };
    if (itQueue != mDirtyQueue.end()) 
    {
        const size_t oldSize { itQueue->second.mPageSize };
        mCurrentDirty += newSize-oldSize;
        itQueue->second.mPageSize = newSize;
        
        PrintDirtyStatus(fname, lock);

        if (newSize > oldSize)
            HandleDirtyMemory(pageMgr, page, true, lock, mgrLock);
    } }
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
            &mPageQueue.back().second.mPageMgr == &pageMgr &&
            mPageQueue.back().first != &page)
        {
            MDBG_INFO("... memory limit! synchronous evict");

            static const std::string fname { __func__ };
            PrintStatus(fname, lock);
            PageInfo pageInfo { mPageQueue.back().second }; // copy

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
            &mDirtyQueue.back().second.mPageMgr == &pageMgr &&
            mDirtyQueue.back().first != &page)
        {
            MDBG_INFO("... dirty limit! synchronous flush");

            static const std::string fname { __func__ };
            PrintDirtyStatus(fname, lock);
            PageInfo pageInfo { mDirtyQueue.back().second }; // copy

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
void CacheManager::RemovePage(const Page& page)
{
    UniqueLock lock(mMutex);
    RemovePage(page, lock);

    static const std::string fname { __func__ };
    PrintStatus(fname, lock);
    PrintDirtyStatus(fname, lock);
}

/*****************************************************/
void CacheManager::RemoveDirty(const Page& page)
{
    UniqueLock lock(mMutex);
    RemoveDirty(page, lock);

    static const std::string fname { __func__ };
    PrintDirtyStatus(fname, lock);
}

/*****************************************************/
size_t CacheManager::RemovePage(const Page& page, const UniqueLock& lock)
{
    size_t pageSize { 0 }; // size of page removed
    PageQueue::lookup_iterator itLookup { mPageQueue.lookup(&page) };
    if (itLookup != mPageQueue.lend())
    {
        MDBG_INFO("(page:" << &page << ")");
        pageSize = itLookup->second->second.mPageSize;
        mCurrentMemory -= pageSize;
        mPageQueue.erase(itLookup);
    }

    RemoveDirty(page, lock);
    return pageSize;
}

/*****************************************************/
void CacheManager::RemoveDirty(const Page& page, const UniqueLock& lock)
{
    PageQueue::lookup_iterator itLookup { mDirtyQueue.lookup(&page) };
    if (itLookup != mDirtyQueue.lend())
    {
        MDBG_INFO("(page:" << &page << ")");
        mCurrentDirty -= itLookup->second->second.mPageSize;
        mDirtyQueue.erase(itLookup);
    }
}

/*****************************************************/
void CacheManager::PrintStatus(const std::string& fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " pages:" << mPageQueue.size() << ", memory:" << mCurrentMemory; });

#if DEBUG // this will kill performance
    size_t total = 0; for (const PageQueue::value_type& pageInfo : mPageQueue) total += pageInfo.second.mPageSize;
    if (total != mCurrentMemory){ MDBG_ERROR(": BAD MEMORY TRACKING! " << total << " != " << mCurrentMemory); assert(false); }
#endif // DEBUG
}

/*****************************************************/
void CacheManager::PrintDirtyStatus(const std::string& fname, const UniqueLock& lock)
{
    mDebug.Info([&](std::ostream& str){ str << fname << "..."
        << " dirtyPages:" << mDirtyQueue.size() << ", dirtyMemory:" << mCurrentDirty; });

#if DEBUG // this will kill performance
    size_t total = 0; for (const PageQueue::value_type& pageInfo : mDirtyQueue) total += pageInfo.second.mPageSize;
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
void CacheManager::DoPageEvictions()
{
    MDBG_INFO("()");

    typedef std::list<std::pair<const Page&, PageInfo>> EvictList;
    // get ScopeLock to make sure pageManager stays in scope between mMutex release and getting pageMgrW lock
    typedef std::pair<PageManager::ScopeLocked, EvictList> EvictSet;
    std::map<PageManager*, EvictSet> currentEvicts;

    { // lock scope
        UniqueLock lock(mMutex);

        static const std::string fname { __func__ };
        PrintStatus(fname, lock);
        size_t cleaned { 0 };

        PageQueue::reverse_iterator pageIt { mPageQueue.rbegin() };
        const size_t margin { mCacheOptions.memoryLimit/mCacheOptions.evictSizeFrac };
        for (; mCurrentMemory + margin > mCacheOptions.memoryLimit + cleaned; ++pageIt)
        {
            const Page& pageRef { *pageIt->first };
            PageInfo& pageInfo { pageIt->second };

            decltype(currentEvicts)::iterator evictIt { currentEvicts.find(&pageInfo.mPageMgr) };
            EvictSet& evictSet { (evictIt != currentEvicts.end()) ? evictIt->second : currentEvicts.emplace(
                &pageInfo.mPageMgr, std::make_pair(pageInfo.mPageMgr.TryLockScope(), EvictList())).first->second };

            if (!evictSet.first) // scope lock
                RemovePage(pageRef, lock); // being deleted
            else
            {
                evictSet.second.emplace_back(pageRef, pageInfo); // copy
                cleaned += pageInfo.mPageSize;
            }
        }
    }

    for (decltype(currentEvicts)::iterator evictIt { currentEvicts.begin() }; evictIt != currentEvicts.end(); )
    {
        EvictSet& evictSet { evictIt->second };
        MDBG_INFO("... evicting pages:" << evictSet.second.size() << " pageMgr:" << evictIt->first);
        
        { // let waiters on this file continue so we can lock
            UniqueLock lock(mMutex);
            mSkipEvictWait = evictIt->first;
            mEvictWaitCV.notify_all();
        }

        SharedLockW mgrLock { evictIt->first->GetWriteLock() };
        
        { // have the mgrLock now
            UniqueLock lock(mMutex);
            mSkipEvictWait = nullptr;
        }

        for (EvictList::value_type& pagePair : evictSet.second)
        {
            const Page& pageRef { pagePair.first };
            const PageInfo& pageInfo { pagePair.second };

            try { pageInfo.mPageMgr.EvictPage(pageInfo.mPageIndex, mgrLock); }
            catch (BaseException& ex)
            {
                UniqueLock lock(mMutex);
                MDBG_ERROR("... " << ex.what());
                
                // move the failed page to the end so we try a different (maybe non-dirty) one next to prevent memory runaway
                if (RemovePage(pageRef, lock)) // only if it's still enqueued (lock was re-acquired)
                    EnqueuePage(pageInfo.mPageMgr, pageInfo.mPageIndex, 
                        pageRef, pageRef.isDirty(), lock);

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

    typedef std::list<std::pair<const Page&, PageInfo>> FlushList;
    // get ScopeLock to make sure pageManager stays in scope between mMutex release and getting pageMgrW lock
    typedef std::pair<PageManager::ScopeLocked, FlushList> FlushSet;
    std::map<PageManager*, FlushSet> currentFlushes;

    { // lock scope
        UniqueLock lock(mMutex);

        static const std::string fname { __func__ };
        PrintDirtyStatus(fname, lock);
        size_t cleaned { 0 };

        PageQueue::reverse_iterator pageIt { mDirtyQueue.rbegin() };
        for (; mCurrentDirty > mDirtyLimit + cleaned; ++pageIt)
        {
            const Page& pageRef { *pageIt->first };
            PageInfo& pageInfo { pageIt->second };

            decltype(currentFlushes)::iterator flushIt { currentFlushes.find(&pageInfo.mPageMgr) };
            FlushSet& flushSet { (flushIt != currentFlushes.end()) ? flushIt->second : currentFlushes.emplace(
                &pageInfo.mPageMgr, std::make_pair(pageInfo.mPageMgr.TryLockScope(), FlushList())).first->second };

            if (!flushSet.first) // scope lock
                RemovePage(pageRef, lock); // being deleted
            else
            {
                flushSet.second.emplace_back(pageRef, pageInfo); // copy
                cleaned += pageInfo.mPageSize;
            }
        }
    }

    for (decltype(currentFlushes)::iterator flushIt { currentFlushes.begin() }; flushIt != currentFlushes.end(); )
    {
        FlushSet& flushSet { flushIt->second };
        MDBG_INFO("... flushing pages:" << flushSet.second.size() << " pageMgr:" << flushIt->first);

        { // let waiters on this file continue so we can lock
            UniqueLock lock(mMutex);
            mSkipFlushWait = flushIt->first;
            mFlushWaitCV.notify_all();
        }

        SharedLockW mgrLock { flushIt->first->GetWriteLock() };
        
        { // have the mgrLock now
            UniqueLock lock(mMutex);
            mSkipFlushWait = nullptr;
        }

        for (FlushList::value_type& pagePair : flushSet.second)
        {
            const PageInfo& pageInfo { pagePair.second };

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

    UniqueLock lock(mMutex); // protect mDirtyLimit
    mDirtyLimit = mBandwidth.UpdateBandwidth(written, std::chrono::steady_clock::now()-timeStart);
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
