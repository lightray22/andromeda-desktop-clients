
#ifndef LIBA2_PAGEMANAGER_H_
#define LIBA2_PAGEMANAGER_H_

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "Page.hpp"

#include "andromeda/Debug.hpp"
#include "andromeda/Semaphor.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class File;

namespace Filedata {

class CacheManager;

/** File page data manager */
class PageManager
{
public:

    /** 
     * Construct a new file page manager
     * @param file reference to the parent file
     * @param fileSize current file size
     * @param pageSize size of pages to use (const)
     */
    PageManager(File& file, const uint64_t fileSize, const size_t pageSize);

    virtual ~PageManager();

    /** Returns the page size in use */
    size_t GetPageSize() const { return mPageSize; }

    /** Returns the current file size including dirty writes */
    uint64_t GetFileSize() const { return mFileSize; }

    /** Returns the current size on the backend (we may have dirty writes) */
    uint64_t GetBackendSize() const { return mBackendSize; }

    /** Returns a read lock for page data */
    SharedLockR GetReadLock() { return SharedLockR(mDataMutex); }
    
    /** Returns a write lock for page data */
    SharedLockW GetWriteLock() { return SharedLockW(mDataMutex); }

    /** Reads data from the given page index into buffer - get lock first! */
    virtual void ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock) final;

    /** Writes data to the given page index from buffer - get lock first! */
    virtual void WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock) final;

    /** Returns true if the page at the given index is dirty */
    bool isDirty(const uint64_t index) const;

    /** Removes the given page, writing it if dirty - get lock first! */
    bool EvictPage(const uint64_t index, const SharedLockW& dataLock);

    /**
     * Writes back all dirty pages - THREAD SAFE
     * @param nothrow if true, don't throw on failure
     */
    void FlushPages(bool nothrow = false);

    /**
     * Informs us of the file changing on the backend - THREAD SAFE
     * @param backendSize new size according to the backend
     */
    void RemoteChanged(const uint64_t backendSize);

    /** Truncate pages according to the given size and inform the backend - THREAD SAFE */
    void Truncate(const uint64_t newSize);

private:

    /** Returns the page at the given index - use GetReadLock() first! */
    const Page& GetPageRead(const uint64_t index, const SharedLockR& dataLock);

    /** 
     * Returns the page at the given index - use GetWriteLock() first! 
     * @param partial if true, pre-populate the page with backend data
     */
    Page& GetPageWrite(const uint64_t index, const bool partial, const SharedLockW& dataLock);

    /** 
     * Resizes an existing page to the given size, informing the CacheManager if inform 
     * CALLER MUST LOCK the DataLockW if operating on an existing page!
     */
    void ResizePage(Page& page, const size_t pageSize, bool inform = true);

    /** Returns true if the page at the given index is pending download */
    bool isPending(const uint64_t index, const UniqueLock& pagesLock);

    /** Returns the read-ahead size to be used for the given VALID (mBackendSize) index */
    size_t GetFetchSize(const uint64_t index, const UniqueLock& pagesLock);

    /** Spawns a thread to read some # of pages at the given VALID (mBackendSize) index */
    void StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock);

    /** Reads count# pages from the backend at the given index */
    void FetchPages(const uint64_t index, const size_t count);

    /** Removes the given index from the pending-read list */
    void RemovePending(const uint64_t index, const UniqueLock& pagesLock);

    /** Map of page index to page pointers */
    typedef std::list<Page*> PagePtrList;

    /** Writes a series of **consecutive** pages (total < size_t) starting at the given index */
    void FlushPageList(const uint64_t index, const PagePtrList& pages, const SharedLockR& dataLock);

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the backend */
    Backend::BackendImpl& mBackend;
    /** Pointer to the cache manager to use */
    CacheManager* mCacheMgr { nullptr };
    /** The size of each page */
    const size_t mPageSize;
    
    /** The current size of the file including dirty extending writes */
    uint64_t mFileSize;
    /** file size as far as the backend knows - may have dirty writes that extend the file */
    uint64_t mBackendSize;

    /** The current read-ahead window */
    size_t mFetchSize { 100 };

    /** Map of page index to page */
    typedef std::map<uint64_t, Page> PageMap;
    /** List of <index,length> pending reads */
    typedef std::list<std::pair<uint64_t, size_t>> PendingMap;

    /** The index based map of pages */
    PageMap mPages;
    /** Unique set of page ranges being downloaded */
    PendingMap mPendingPages;
    /** Condition variable for waiting for pages */
    std::condition_variable mPagesCV;
    /** Mutex that protects the page maps */
    mutable std::mutex mPagesMutex;

    /** 
     * Mutex that protects page content and mBackendSize 
     * Specifically we must use a reader-preference lock - when doing ReadPage with the R lock, we
     * spawn the background FetchPages thread which also gets its own R lock until the read-ahead is done.
     * If someone were to acquire a W lock (W2) after ReadPage (R1) but before FetchPages (R3) then we would deadlock
     */
    SharedMutex mDataMutex;

    Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
