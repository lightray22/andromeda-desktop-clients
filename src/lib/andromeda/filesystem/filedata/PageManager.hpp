
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
#include "andromeda/typedefs.hpp"

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
     * @param fileSize current file size
     * @param pageSize size of pages to use (const)
     */
    PageManager(File& file, Backend::BackendImpl& backend, const uint64_t fileSize, const size_t pageSize);

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

    /** Reads data from the given page index into buffer */
    virtual void ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock) final;

    /** Writes data to the given page index from buffer */
    virtual void WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock) final;

    /** Returns true if the page at the given index is dirty */
    bool isDirty(const uint64_t index) const;

    /** Removes the page at the given index, writing it if dirty - THREAD SAFE */
    bool EvictPage(const uint64_t index);

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

    /** Map of page index to page */
    typedef std::map<uint64_t, Page> PageMap;
    /** Map of page index to page reference */
    typedef std::map<uint64_t, Page&> PageRefMap;
    /** List of <index,length> pending reads */
    typedef std::list<std::pair<uint64_t, size_t>> PendingMap;

    /** Returns the page at the given index - use GetReadLock() first! */
    const Page& GetPageRead(const uint64_t index, const SharedLockR& dataLock);

    /** 
     * Returns the page at the given index - use GetWriteLock() first! 
     * @param partial if true, pre-populate the page with backend data
     */
    Page& GetPageWrite(const uint64_t index, const bool partial, const SharedLockW& dataLock);

    /** Resizes an existing page to the given size */
    void ResizePage(Page& page, uint64_t pageSize, const SharedLockW& dataLock);

    /** Returns true if the page at the given index is pending download */
    bool isPending(const uint64_t index, const UniqueLock& pagesLock);

    /** Returns the read-ahead size to be used for the given VALID (mBackendSize) index */
    size_t GetFetchSize(const uint64_t index, const UniqueLock& pagesLock);

    /** Spawns a thread to read some # of pages at the given VALID (mBackendSize) index */
    void StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock);

    /** Reads count# pages from the backend at the given index */
    void FetchPages(const uint64_t index, const size_t count, SharedLockR threadsLock);

    /** Removes the given index from the pending-read list */
    void RemovePending(const uint64_t index, const UniqueLock& pagesLock);

    /** Writes a series of consecutive pages */
    void FlushPageList(PageRefMap& pages, const SharedLockR& dataLock);

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the backend */
    Backend::BackendImpl& mBackend;
    /** The size of each page */
    const size_t mPageSize;
    
    /** The current size of the file including dirty extending writes */
    uint64_t mFileSize;
    /** file size as far as the backend knows - may have dirty writes that extend the file */
    uint64_t mBackendSize;

    /** The current read-ahead window */
    size_t mFetchSize { 100 };

    /** The index based map of pages */
    PageMap mPages;
    /** Unique set of page ranges being downloaded */
    PendingMap mPendingPages;
    /** Condition variable for waiting for pages */
    std::condition_variable mPagesCV;
    /** Mutex that protects the page maps */
    mutable std::mutex mPagesMutex;

    /** Mutex that protects page content and mBackendSize */
    std::shared_mutex mDataMutex;

    /** Mutex held by background threads to keep this in scope */
    std::shared_mutex mClassMutex;
    
    Andromeda::Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
