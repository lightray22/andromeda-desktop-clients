
#ifndef LIBA2_PAGEMANAGER_H_
#define LIBA2_PAGEMANAGER_H_

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <thread>

#include "Page.hpp"

#include "andromeda/Debug.hpp"
#include "andromeda/Semaphor.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class File;

namespace Filedata {

/** File page data manager */
class PageManager // TODO comments
{
public:

    PageManager(File& file, Backend::BackendImpl& backend, size_t pageSize);

    virtual ~PageManager();

    size_t GetPageSize() const { return mPageSize; }

    const PageReader GetPageReader(const uint64_t index);

    PageWriter GetPageWriter(const uint64_t index);

    void DeletePage(const uint64_t index);

    void Truncate(const uint64_t newSize);

private:

    typedef std::unique_lock<std::mutex> UniqueLock;
    typedef std::shared_lock<std::shared_mutex> SharedLock;

    /** A map of page index to shared_ptr to a page */
    typedef std::map<uint64_t, SharedPage> PageMap; 
    typedef std::list<std::thread> ThreadList;

    SharedPage& GetPage(const uint64_t index, UniqueLock& pages_lock);

    void FetchPage(const uint64_t index, SemaphorLock thread_count, SharedLock threads_lock);

    PageMap::iterator ErasePage(PageMap::iterator& it);

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the backend */
    Backend::BackendImpl& mBackend;

    const size_t mPageSize;
    // TODO maybe store file size here? take it out of item, no need to keep folder sizes...
    // also seems like backend size should live here too...

    size_t mReadAhead { 10 }; // TODO fixed for now

    /** The index based map of pages */
    PageMap mPages;
    /** Unique set of pages being downloaded */
    std::set<uint64_t> mPendingPages;
    /** Condition variable for waiting for pages */
    std::condition_variable mPagesCV;
    /** Mutex that protects mPages and mPendingPages */
    std::mutex mPagesMutex;

    /** Mutex held by background threads to keep this in scope */
    std::shared_mutex mThreadsMutex;

    Andromeda::Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
