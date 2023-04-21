
#ifndef LIBA2_CACHEMANAGER_H_
#define LIBA2_CACHEMANAGER_H_

#include <atomic>
#include <condition_variable>
#include <exception>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "BandwidthMeasure.hpp"
#include "Page.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

struct Page;
class PageManager;
struct CacheOptions;

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
    class MemoryException : public BaseException { public:
        /** @param message API error message */
        explicit MemoryException(const std::string& type) :
            BaseException("Failed to reserve memory: "+type+" error") {}; };

    /** If true, start the cleanup threads immediately */
    explicit CacheManager(const CacheOptions& cacheOptions, bool startThreads = true);

    virtual ~CacheManager();

    /** Runs the cleanup threads */
    void StartThreads();

    /** Returns the maximum cache memory size */
    uint64_t GetMemoryLimit() const;
    
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
     * @throws MemoryException if canWait and evict/flush fails to free memory
     * @throws BaseException if canWait and flush for this pageMgr fails to free memory
     */
    void InformPage(PageManager& pageMgr, const uint64_t index, const Page& page, bool dirty,
        bool canWait = true, const SharedLockW* mgrLock = nullptr);

    /**
     * Inform us that a page has changed size
     * if mgrLock is given, may synchronously evict or flush pages on this manager
     * @param pageMgr the page manager that owns the page
     * @param page reference to the page
     * @param newSize the new size of the page
     * @param mgrLock the W lock for the page manager if available
     * @throws MemoryException if evict/flush fails to free memory
     * @throws BaseException if flush for this pageMgr fails to free memory
     */
    void ResizePage(const PageManager& pageMgr, const Page& page, const size_t newSize, 
        const SharedLockW* mgrLock = nullptr);

    /** Inform us that a page has been erased */
    void RemovePage(const Page& page);

    /** Inform us that a page is no longer dirty */
    void RemoveDirty(const Page& page);
    
private:

    typedef std::unique_lock<std::mutex> UniqueLock;

    /** Returns true if we should wait for a page eviction */
    inline bool ShouldAwaitEvict(const PageManager& pageMgr, const UniqueLock& lock);

    /** Returns true if we should wait for a page flushing */
    inline bool ShouldAwaitFlush(const PageManager& pageMgr, const UniqueLock& lock);

    /**
     * Signals the evict thread and checks memory
     * @param pageMgr pageManager that is making this call
     * @param page do not synchronously evict this page
     * @param canWait wait if memory is not below limits
     * @param mgrLock the W lock for the page manager if available
     */
    void HandleMemory(const PageManager& pageMgr, const Page& page, 
        bool canWait, UniqueLock& lock, const SharedLockW* mgrLock = nullptr);

    /**
     * Signals the flush thread and checks dirty memory
     * @param pageMgr pageManager that is making this call
     * @param page do not synchronously flush this page
     * @param canWait wait if memory is not below limits
     * @param mgrLock the W lock for the page manager if available
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
    size_t EnqueuePage(PageManager& pageMgr, const uint64_t index, const Page& page, bool dirty, const UniqueLock& lock);

    /** 
     * Inform us that a page has been erased (already have the lock) 
     * @return size_t size of the page that was erased or 0 if it didn't exist
     */
    size_t RemovePage(const Page& page, const UniqueLock& lock);

    /** Inform us that a page is no longer dirty (already have the lock) */
    void RemoveDirty(const Page& page, const UniqueLock& lock);

    /** Send some stats about memory to debug */
    void PrintStatus(const char* const fname, const UniqueLock& lock);

    /** Send some stats about the dirty memory to debug */
    void PrintDirtyStatus(const char* const fname, const UniqueLock& lock);

    /** Run the page evict task in a loop while mRunCleanup */
    void EvictThread();
    /** Run the page flush task in a loop while mRunCleanup */
    void FlushThread();

    /** Run necessary page evictions */
    inline void DoPageEvictions();
    /** Run necessary page flushes */
    inline void DoPageFlushes();

    /** Calls flush on a page and updates the bandwidth measurement */
    template<class T>
    void FlushPage(PageManager& pageMgr, const uint64_t index, const T& mgrLock);

    /** Mutex to guard writing data structures */
    std::mutex mMutex;

    typedef struct
    {
        /** Reference to the page manager owner of the page */
        PageManager& mPageMgr;
        /** Index of the page in the pageMgr */
        const uint64_t mPageIndex;
        /** Reference to the page object */
        const Page& mPageRef;
        /** Size of the page when it was added */
        size_t mPageSize;
    } PageInfo;

    /** LIFO queue of pages ordered OLD->NEW */
    typedef std::list<PageInfo> PageList;
    PageList mPageQueue;
    PageList mDirtyQueue;

    /** HashMap allowing efficient lookup of pages within the queue */
    typedef std::unordered_map<const Page*, PageList::iterator> PageItMap; 
    PageItMap mPageItMap; 
    PageItMap mDirtyItMap;

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
    uint64_t mCurrentMemory { 0 };

    /** The maximum in-memory dirty page usage before flushing (dynamic) */
    uint64_t mDirtyLimit { 0 };
    /** The current total dirty page memory */
    uint64_t mCurrentDirty { 0 };

    /** Exception encountered while evicting */
    std::exception_ptr mEvictFailure;
    /** Exception encountered while flushing */
    std::exception_ptr mFlushFailure;

    Debug mDebug;

    /** Bandwidth measurement tool for mDirtyLimit */
    BandwidthMeasure mBandwidth;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHEMANAGER_H_
