
#ifndef LIBA2_PAGEMANAGER_H_
#define LIBA2_PAGEMANAGER_H_

#include <array>
#include <chrono>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <thread>

#include "BandwidthMeasure.hpp"
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
     * @param backendExists true if the file exists on the backend
     */
    PageManager(File& file, const uint64_t fileSize, const size_t pageSize, bool backendExists);

    virtual ~PageManager();

    /** Returns the page size in use */
    size_t GetPageSize() const { return mPageSize; }

    /** Returns the current file size including dirty writes */
    uint64_t GetFileSize() const { return mFileSize; }

    /** Returns the current size on the backend (we may have dirty writes) */
    uint64_t GetBackendSize() const { return mBackendSize; }

    /** Returns whether or not the file exists on the backend */
    bool ExistsOnBackend() const { return mBackendExists; }

    /** Returns a read lock for page data */
    SharedLockR GetReadLock() { return SharedLockR(mDataMutex); }
    
    /** Returns a priority read lock for page data */
    SharedLockRP GetReadPriLock() { return SharedLockRP(mDataMutex); }
    
    /** Returns a write lock for page data */
    SharedLockW GetWriteLock() { return SharedLockW(mDataMutex); }

    typedef std::shared_lock<std::shared_mutex> ScopeLock;

    /** Tries to lock mScopeMutex, returns a lock object that is maybe locked */
    ScopeLock TryGetScopeLock() { return ScopeLock(mScopeMutex, std::try_to_lock); }

    /** Reads data from the given page index into buffer - get lock first! */
    virtual void ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock) final;

    /** Writes data to the given page index from buffer - get lock first! */
    virtual void WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock) final;

    /** Returns true if the page at the given index is dirty */
    bool isDirty(const uint64_t index) const;

    /** Removes the given page, writing it if dirty - get lock first! */
    bool EvictPage(const uint64_t index, const SharedLockW& dataLock);

    /** 
     * Flushes the given page if dirty - get lock first!
     * @return the total number of bytes written to the backend
     */
    size_t FlushPage(const uint64_t index, const SharedLockRP& dataLock);

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

    typedef std::unique_lock<std::mutex> UniqueLock;

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

    /** 
     * Returns the read-ahead size to be used for the given VALID (mFileSize) index 
     * Returns 0 if the index does not exist on the backend (see mBackendSize)
     */
    size_t GetFetchSize(const uint64_t index, const UniqueLock& pagesLock);

    /** Spawns a thread to read some # of pages at the given VALID (mBackendSize) index */
    void StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock);

    /** 
     * Reads count# pages from the backend at the given index, adding to the page map
     * Does NOT get a dataLock or inform the cacheManager of the new page if count is 1!
     */
    void FetchPages(const uint64_t index, const size_t count);

    /** Removes the given index from the pending-read list */
    void RemovePending(const uint64_t index, const UniqueLock& pagesLock);

    /** Updates mFetchSize with the given bandwidth measurement - THREAD SAFE */
    void UpdateBandwidth(const size_t bytes, const std::chrono::steady_clock::duration& time);

    /** Map of page index to page */
    typedef std::map<uint64_t, Page> PageMap;
    /** List of **consecutive** page pointers */
    typedef std::list<Page*> PagePtrList;

    /** 
     * Returns a series of **consecutive** dirty pages (total bytes < size_t)
     * @param pageIt reference to the iterator to start with - will end as the next index not used
     * @param writeList reference to a list of pages to fill out - guaranteed not empty if pageIt is dirty
     * @return uint64_t the start index of the write list (not valid if writeList is empty!)
     */
    uint64_t GetWriteList(PageMap::iterator& pageIt, PagePtrList& writeList, const UniqueLock& pagesLock);

    /** 
     * Writes a series of **consecutive** pages (total < size_t) starting at the given index - MUST HAVE DATALOCKR/W!
     * Also marks each page not dirty and informs the cache manager
     * Also creates the file on the backend if necessary (see mBackendExists)
     * @param pages list of pages to flush - must NOT be empty
     * @return the total number of bytes written to the backend
     */
    size_t FlushPageList(const uint64_t index, const PagePtrList& pages, const UniqueLock& flushLock);

    /** 
     * Asserts the file is created on the backend (see mBackendExists) - MUST HAVE DATALOCKR/W!
     * Also calls truncate if max(mBackendSize,maxDirty) < mFileSize 
     */
    void FlushTruncate(const UniqueLock& flushLock);

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
    /** true iff the file has been created on the backend (false if waiting for flush) */
    bool mBackendExists;

    /** The current read-ahead window (number of pages) - never 0 */
    size_t mFetchSize { 1 };
    /** Mutex that protects mFetchSize and mBandwidthHistory */
    std::mutex mFetchSizeMutex;
    /** The maximum fraction of the cache that a read-ahead can consume (1/x) */
    const size_t mReadMaxCacheFrac { 4 };

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

    /** Shared mutex that protects page content/buffers and mBackendSize */
    SharedMutex mDataMutex;
    /** Shared mutex that is grabbed exclusively when this class is destructed */
    std::shared_mutex mScopeMutex;
    /** Don't want overlapping flushes, could duplicate writes */
    std::mutex mFlushMutex;

    Debug mDebug;

    /** Bandwidth measurement tool for mFetchSize */
    BandwidthMeasure mBandwidth;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
