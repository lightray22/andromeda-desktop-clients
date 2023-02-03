
#ifndef LIBA2_CACHEMANAGER_H_
#define LIBA2_CACHEMANAGER_H_

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "BandwidthMeasure.hpp"
#include "Page.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

struct Page;
class PageManager;

/** 
 * Manages pages as an LRU cache to limit memory usage 
 * Also tracks dirty pages to limit the total dirty memory
 */
class CacheManager
{
public:

    CacheManager();

    virtual ~CacheManager();

    /** Returns the maximum cache memory size */
    uint64_t GetMemoryLimit() const { return mMemoryLimit; }

    /** 
     * Inform us that a page was used, putting at the front of the LRU
     * @param pageMgr the page manager that owns the page
     * @param index the page manager page index
     * @param page reference to the page
     * @param canWait block if memory is not available
     */
    void InformPage(PageManager& pageMgr, const uint64_t index, const Page& page, bool canWait = true);

    /** Inform us that a page has changed size */
    void ResizePage(const Page& page, const size_t newSize);

    /** Inform us that a page has been erased */
    void RemovePage(const Page& page);

    /** Inform us that a page is no longer dirty */
    void RemoveDirty(const Page& page);
    
private:

    typedef std::unique_lock<std::mutex> UniqueLock;

    /** 
     * Inform us that a page has been erased (already have the lock) 
     * @return size_t size of the page that was erased or 0 if didn't exist
     */
    size_t RemovePage(const Page& page, const UniqueLock& lock);

    /** Inform us that a page is no longer dirty (already have the lock) */
    void RemoveDirty(const Page& page, const UniqueLock& lock);

    /** Send some stats about memory to debug */
    void PrintStatus(const char* const fname, const UniqueLock& lock);

    /** Send some stats about the dirty memory to debug */
    void PrintDirtyStatus(const char* const fname, const UniqueLock& lock);

    /** Run the page cleanup loop */
    void CleanupThread();

    /** Mutex to guard writing data structures */
    std::mutex mMutex;

    typedef struct
    {
        /** Reference to the page manager owner of the page */
        PageManager& mPageMgr;
        /** Index of the page in the pageMgr */
        const uint64_t mPageIndex;
        /** Pointer to the page object */
        const Page* mPagePtr;
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
    bool mRunCleanup { true };

    /** Background page cleanup thread */
    std::thread mThread;
    /** CV to wait/signal page cleanup thread */
    std::condition_variable mThreadCV;
    /** CV to signal when memory is available */
    std::condition_variable mMemoryCV;
    /** CV to signal when dirty space is available */
    std::condition_variable mDirtyCV;

    /** PageManager that can skip the memory wait (need it to clear its lock queue) */
    PageManager* mSkipMemoryWait { nullptr };

    /** The maximum page memory usage before evicting */
    const uint64_t mMemoryLimit { 256*1024*1024 };
    /** Fraction of mMemoryLimit to remove when evicting */
    const size_t mMemoryMarginFrac { 16 };
    /** The current total memory usage */
    uint64_t mCurrentMemory { 0 };

    /** The maximum in memory dirty page usage before flushing - default 128K */
    uint64_t mDirtyLimit { 0 };
    /** The current total dirty page memory */
    uint64_t mCurrentDirty { 0 };

    Debug mDebug;

    /** Bandwidth measurement tool for mDirtyLimit */
    BandwidthMeasure mBandwidth;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHEMANAGER_H_
