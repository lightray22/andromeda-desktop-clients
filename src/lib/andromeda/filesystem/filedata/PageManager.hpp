
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
class PageManager
{
public:

    /** 
     * Construct a new file page manager
     * @param file reference to the parent file
     * @param backend reference to the backend
     * @param pageSize size of pages to use (const)
     */
    PageManager(File& file, Backend::BackendImpl& backend, const size_t pageSize);

    virtual ~PageManager();

    /** Returns the page size in use */
    size_t GetPageSize() const { return mPageSize; }

    /** Returns the current size on the backend (we may have dirty writes) */
    uint64_t GetBackendSize() const { return mBackendSize; }

    /** Returns a locked read page at the given index - must exist */
    const PageReader GetPageReader(const uint64_t index);

    /**
     * Returns a locked write page at the given index - can be a new index
     * @param offset page offset of the intended write
     * @param length length of the intended write
     */
    PageWriter GetPageWriter(const uint64_t index, const size_t offset, const size_t length);

    /** Returns true if the page at the given index is dirty */
    bool isDirty(const uint64_t index);

    /** Removes the page at the given index, writing it if dirty */
    bool EvictPage(const uint64_t index);

    /**
     * Removes all non-dirty pages
     * @return uint64_t the max dirty offset + 1
     */
    uint64_t EvictReadPages();

    /**
     * Writes back all dirty pages
     * @param nothrow if true, don't throw on failure
     */
    void FlushPages(bool nothrow = false);

    /**
     * Informs us of the file changing on the backend
     * 
     * @param backendSize new size according to the backend
     * @param reset if true, invalidate/dump all pages
     * @return uint64_t the max dirty offset + 1
     */
    uint64_t RemoteChanged(const uint64_t backendSize, bool reset = true);

    /** Truncate pages according to the given size and inform the backend */
    void Truncate(const uint64_t newSize);

private:

    typedef std::unique_lock<std::mutex> UniqueLock;
    typedef std::shared_lock<std::shared_mutex> SharedLockR;
    typedef std::unique_lock<std::shared_mutex> SharedLockW;

    /** A unique_ptr to a page - Page cannot be moved due to mutex */
    typedef std::unique_ptr<Page> PagePtr;
    /** Map of page index to page */
    typedef std::map<uint64_t, PagePtr> PageMap;
    /** List of <index,length> pending reads */
    typedef std::list<std::pair<uint64_t, size_t>> PendingMap;

    /** Returns true if the page at the given index is dirty */
    bool isDirty(const uint64_t index, UniqueLock& pagesLock);

    /** Returns true if the page at the given index is pending download */
    bool isPending(const uint64_t index, UniqueLock& pagesLock);

    /** Spawns a thread to read a single page at the given index */
    void StartRead1(const uint64_t index, UniqueLock& pagesLock);

    /** Spawns a thread to read some # of pages at the given index */
    void StartReadN(const uint64_t index, UniqueLock& pagesLock);

    /** Returns the read-ahead size to be used for the given indxe */
    uint64_t GetReadSize(const uint64_t index, UniqueLock& pagesLock);

    /** Reads count# pages from the backend at the given index */
    void ReadPages(const uint64_t index, const size_t count, SharedLockR threadsLock);

    /** Removes the given index from the pending-read list */
    void RemovePending(const uint64_t index, UniqueLock& pagesLock);

    /** A map of page offset to a locked page */
    typedef std::map<uint64_t, PageReader> LockedPageMap;

    /** Writes a series of consecutive locked pages */
    void WritePages(LockedPageMap& pages);

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the backend */
    Backend::BackendImpl& mBackend;
    /** The size of each page */
    const size_t mPageSize;
    
    /* file size as far as the backend knows - may have dirty writes that extend the file */
    uint64_t mBackendSize { 0 };
    /** The current read-ahead window including current */
    size_t mReadSize { 100 };

    /** The index based map of pages */
    PageMap mPages;
    /** Unique set of page ranges being downloaded */
    PendingMap mPendingPages;
    /** Condition variable for waiting for pages */
    std::condition_variable mPagesCV;
    /** Mutex that protects mPages and mPendingPages */
    std::mutex mPagesMutex;

    /** Mutex held by background threads to keep this in scope */
    std::shared_mutex mClassMutex;
    /** Mutex held by background threads to allow running */
    std::shared_mutex mReadersMutex;

    Andromeda::Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
