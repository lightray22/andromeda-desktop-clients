
#ifndef LIBA2_CACHEMANAGER_H_
#define LIBA2_CACHEMANAGER_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>

#include "BandwidthMeasure.hpp"
#include "CacheOptions.hpp"
#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/OrderedMap.hpp"
#include "andromeda/ScopeLocked.hpp"
#include "andromeda/SharedMutex.hpp"
#include "andromeda/filesystem/Item.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

class Page;
class PageManager;
class CachingAllocator;

/** 
 * Manages pages as an LRU cache to limit memory usage, by calling EvictPage()
 * Also tracks dirty pages to limit the total dirty memory, by calling FlushPage()
 * The maximum dirty pages is in terms of time, determined by bandwidth measurement
 * Fully thread-safe. Evict/Flush are synchronous if possible when writing for
 *     error-catching - otherwise, they happen on background threads.
 * Callers adding new/bigger pages will block until memory is available
 * THREAD SAFE (INTERNAL LOCKS) + external PageManager locking
 */
class CacheManager
{
public:

    /** Exception indicating memory could not be reserved due to a evict/flush failure */
    class MemoryException : public Item::Exception { public:
        /** @param message API error message */
        explicit MemoryException(const std::string& type) :
            Item::Exception("Failed to reserve memory: "+type+" error") {}; };

    /** If true, start the cleanup threads immediately */
    explicit CacheManager(const CacheOptions& cacheOptions, bool startThreads = true);

    /** Stop cleanup threads and destruct - ALL page activity must be stopped! */
    virtual ~CacheManager();
    DELETE_COPY(CacheManager)
    DELETE_MOVE(CacheManager)

    /** Runs the cleanup threads */
    void StartThreads();

    /** Returns the maximum cache memory size */
    size_t GetMemoryLimit() const;

    struct Stats { 
        size_t currentTotal; size_t totalPages; 
        size_t currentDirty; size_t dirtyLimit; size_t dirtyPages; };
    /** Returns a copy of some member variables for debugging */
    inline Stats GetStats() const { 
        const UniqueLock lock(mMutex); return { 
            mCurrentTotal, mPageQueue.size(), 
            mCurrentDirty, mDirtyLimit, mDirtyQueue.size() }; }

    /** Returns the allocator to use for all file data */
    inline CachingAllocator& GetPageAllocator(){ return *mPageAllocator; }
    
    /** 
     * Inform us that a page was used, putting at the front of the LRU
     * if mgrLock is given, may synchronously evict or flush pages on this manager
     * IF this fails, the caller must call RemovePage() or ResizePage(oldSize)
     * @param pageMgr the page manager that owns the page
     * @param index the page manager page index
     * @param page reference to the page
     * @param dirty if true, consider dirty memory also
     * @param canWait wait if memory is not below limits
     * @param mgrLock the W lock for the page manager if available
     * @throws BackendException if canWait and synchronous failure to free memory
     * @throws MemoryException if canWait and non-synchronous failure to free memory
     */
    void InformPage(PageManager& pageMgr, uint64_t index, const Page& page, bool dirty,
        bool canWait = true, const SharedLockW* mgrLock = nullptr);

    /**
     * Inform us that a page has changed size
     * if mgrLock is given, may synchronously evict or flush pages on this manager
     * IF this fails, the caller must call RemovePage() or ResizePage(oldSize)
     * @param pageMgr the page manager that owns the page
     * @param page reference to the page with its new size set
     * @param mgrLock the W lock for the page manager if available
     * @throws BackendException if synchronous failure to free memory
     * @throws MemoryException if non-synchronous failure to free memory
     */
    void ResizePage(const PageManager& pageMgr, const Page& page, const SharedLockW* mgrLock = nullptr);

    /** Inform us that a page has been erased */
    void RemovePage(const Page& page);

    /** Inform us that a page is no longer dirty */
    void RemoveDirty(const Page& page);
    
private:

    using UniqueLock = std::unique_lock<std::mutex>;

    /** 
     * Returns true if we should wait for a page eviction
     * @throws MemoryException if over limit and mEvictFailure is set
     */
    inline bool ShouldAwaitEvict(const PageManager& pageMgr, const UniqueLock& lock) const;

    /** 
     * Returns true if we should wait for a page flushing
     * @throws MemoryException if over limit and mFlushFailure is set
     */
    inline bool ShouldAwaitFlush(const PageManager& pageMgr, const UniqueLock& lock) const;

    /** Returns true if evict should run (memory is over the limit) */
    inline bool ShouldEvict(const UniqueLock& lock) const { return mCurrentTotal > mCacheOptions.memoryLimit; }

    /** Returns true if flush should run (dirty memory is over the limit) */
    inline bool ShouldFlush(const UniqueLock& lock) const { return mCurrentDirty > mDirtyLimit; }

    /** Returns true if evict/flush waiting should be skipped for the given page manager */
    inline bool ShouldSkipWait(const PageManager& pageMgr, const UniqueLock& lock) const
    {
        return mSkipEvictWait == &pageMgr || mSkipFlushWait == &pageMgr;
    }

    /**
     * Signals the evict thread and checks memory
     * @param pageMgr pageManager that is making this call
     * @param page do not synchronously evict this page
     * @param canWait wait if memory is not below limits
     * @param mgrLock the W lock for the page manager if available
     * @throws BackendException on synchronous failure if canWait
     * @throws MemoryException on non-synchronous failure if canWait
     */
    void HandleMemory(const PageManager& pageMgr, const Page& page, 
        bool canWait, UniqueLock& lock, const SharedLockW* mgrLock = nullptr);

    /**
     * Signals the flush thread and checks dirty memory
     * @param pageMgr pageManager that is making this call
     * @param page do not synchronously flush this page
     * @param canWait wait if memory is not below limits
     * @param mgrLock the W lock for the page manager if available
     * @throws BackendException on synchronous failure if canWait
     * @throws MemoryException on non-synchronous failure if canWait
     */
    void HandleDirtyMemory(const PageManager& pageMgr, const Page& page, 
        bool canWait, UniqueLock& lock, const SharedLockW* mgrLock = nullptr);

    /** 
     * Inform us that a page was used, putting at the front of the LRU
     * @param pageMgr the page manager that owns the page
     * @param index the page manager page index
     * @param page reference to the page
     * @param dirty if true, consider dirty memory also
     * @return size_t the size of the old page or 0 if it didn't exist
     */
    size_t EnqueuePage(PageManager& pageMgr, uint64_t index, const Page& page, bool dirty, const UniqueLock& lock);

    /** 
     * Inform us that a page has been erased (already have the lock) 
     * @return size_t size of the page that was erased or 0 if it didn't exist
     */
    size_t RemovePage(const Page& page, const UniqueLock& lock);

    /** Inform us that a page is no longer dirty (already have the lock) */
    void RemoveDirty(const Page& page, const UniqueLock& lock);

    /** Send some stats about memory to debug */
    void PrintStatus(const char* fname, const UniqueLock& lock);

    /** Send some stats about the dirty memory to debug */
    void PrintDirtyStatus(const char* fname, const UniqueLock& lock);

    /**
     * Returns an exclusive lock for the given page manager, with deadlock avoidance
     * @param waitPtr ref to mSkipEvictWait or mSkipFlushWait for deadlock avoidable
     */
    SharedLockW GetPageManagerLock(PageManager& pageMgr, PageManager*& waitPtr);

    /** Run the page evict task in a loop while mRunCleanup */
    void EvictThread();
    /** Run the page flush task in a loop while mRunCleanup */
    void FlushThread();

    /** 
     * Run necessary page evictions
     * Sets mEvictFailure on BackendException
     */
    inline void DoPageEvictions() noexcept;
    /** 
     * Run necessary page flushes
     * Sets mFlushFailure on BackendException
     */
    inline void DoPageFlushes() noexcept;

    /** 
     * Calls flush on a page and updates the bandwidth measurement
     * @throws BackendException for backend issues
     */
    void FlushPage(PageManager& pageMgr, uint64_t index, const SharedLockW& mgrLock);

    mutable Debug mDebug;

    /** Mutex to guard writing data structures */
    mutable std::mutex mMutex;

    using PageInfo = struct
    {
        /** Reference to the page manager owner of the page */
        PageManager& mPageMgr;
        /** Index of the page in the pageMgr */
        const uint64_t mPageIndex;
        /** Size of the page when it was added */
        size_t mPageSize;
    };

    /** LIFO queue of pages for an LRU cache */
    using PageQueue = OrderedMap<const Page*, PageInfo>;
    PageQueue mPageQueue;
    PageQueue mDirtyQueue;

    // structures used in Page Evict/Flush
    using PageList = std::list<std::pair<const Page&, PageInfo>>;
    using LockedPageList = std::pair<ScopeLocked<PageManager>, PageList>;
    using PageMgrPageMap = std::map<PageManager*, LockedPageList>;

    /** Set to false to stop the cleanup threads */
    std::atomic<bool> mRunCleanup { true };

    /** Background page eviction thread */
    std::thread mEvictThread;
    /** CV to wait/signal page eviction thread */
    std::condition_variable mEvictThreadCV;
    /** CV to signal when memory is available */
    std::condition_variable mEvictWaitCV;
    
    /** Background page flushing thread */
    std::thread mFlushThread;
    /** CV to wait/signal page flushing thread */
    std::condition_variable mFlushThreadCV;
    /** CV to signal when dirty memory is available */
    std::condition_variable mFlushWaitCV;

    /** PageManager that can skip the evict wait (need it to clear its lock queue) */
    PageManager* mSkipEvictWait { nullptr };
    /** PageManager that can skip the flush wait (need it to clear its lock queue) */
    PageManager* mSkipFlushWait { nullptr };

    /** Reference to CacheOptions */
    const CacheOptions& mCacheOptions;

    /** The current total memory usage */
    size_t mCurrentTotal { 0 };

    /** The maximum in-memory dirty page usage before flushing (dynamic) */
    size_t mDirtyLimit { 0 };
    /** The current total dirty page memory */
    size_t mCurrentDirty { 0 };

    /** Exception encountered while evicting */
    std::exception_ptr mEvictFailure;
    /** Exception encountered while flushing */
    std::exception_ptr mFlushFailure;

    /** Bandwidth measurement tool for mDirtyLimit */
    BandwidthMeasure mBandwidth;
    /** Allocator to use for all file pages (never null) */
    std::unique_ptr<CachingAllocator> mPageAllocator;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHEMANAGER_H_
