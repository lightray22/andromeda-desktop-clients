
#ifndef LIBA2_CACHEMANAGER_H_
#define LIBA2_CACHEMANAGER_H_

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "Page.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

struct Page;
class PageManager;

/** Manages pages as an LRU cache to limit memory usage */
class CacheManager
{
public:

    CacheManager();

    virtual ~CacheManager();

    /** 
     * Inform us that a page was used, putting at the front of the LRU
     * @param pageMgr the page manager that owns the page
     * @param index the page manager page index
     * @param page reference to the page
     */
    void InformPage(PageManager& pageMgr, const uint64_t index, const Page& page);

    /** Inform us that a page has changed size */
    void ResizePage(const Page& page, const size_t newSize);

    /** Inform us that a page has been erased */
    void ErasePage(const Page& page);
    
private:

    /** Inform us that a page has been erased (already have the lock) */
    void ErasePage(const Page& page, const UniqueLock& lock);

    /** Send some stats to debug */
    void PrintStatus(const char* const fname, const UniqueLock& lock);

    /** Run the main cache management loop */
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
        /** Size of the page when it was added, in case it changes without informing */
        size_t mPageSize;
    } PageInfo;

    /** LIFO queue of pages ordered OLD->NEW */
    typedef std::list<PageInfo> PageList; PageList mPageQueue;
    /** HashMap allowing efficient lookup of pages within the queue */
    typedef std::unordered_map<const Page*, PageList::iterator> PageItMap; PageItMap mPageItMap;

    /** List of pages currently being evicted (per-page manager) to ignore adding */
    typedef std::unordered_map<PageManager*, PageList> PageMgrLists; PageMgrLists mCurrentEvicts;

    /** Background cleanup thread */
    std::thread mThread;
    /** CV to wait/signal cleanup thread */
    std::condition_variable mThreadCV;
    /** Set to false to stop the cleanup thread */
    bool mRunCleanup { true };
    /** Indicates the cleanup thread is active */
    bool mCleanupActive { false };

    // TODO max memory/limit margin configuration/real defaults

    /** The maximum page memory usage before evicting */
    const uint64_t mMemoryLimit { 200*1024*1024 };
    /** Amount to below limit to get to when evicting */
    const size_t mLimitMargin { 20*1024*1024 };
    /** The current total memory usage */
    uint64_t mCurrentMemory { 0 };

    Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_CACHEMANAGER_H_
