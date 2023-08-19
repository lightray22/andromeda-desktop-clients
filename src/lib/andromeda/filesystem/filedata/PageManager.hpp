
#ifndef LIBA2_PAGEMANAGER_H_
#define LIBA2_PAGEMANAGER_H_

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <list>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "BandwidthMeasure.hpp"
#include "PageBackend.hpp"

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ScopeLocked.hpp"
#include "andromeda/SharedMutex.hpp"

#include "andromeda/filesystem/File.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
namespace Filedata {

class CacheManager;
class Page;

/** 
 * File page data manager - splits the file into a series of fixed size pages
 * Implements thread-safe interfaces to read, write, truncate, evict and flush
 * Implements various tricks/caching to greatly increase speed:
 *  - caches pages read from the backend (see EvictPage)
 *  - reads ahead consecutive ranges of pages sized by bandwidth,
 *      doing so on a background thread to minimize waiting
 *  - caches writes until flushed (write-back cache) (see FlushPage)
 *  - writes back consecutive ranges of pages to maximize throughput
 *  - supports delayed file Create to combine Create+Write to Upload
 * THREAD SAFE (FORCES EXTERNAL LOCKS) (use parent File's lock)
 */
class PageManager
{
public:

    /** 
     * Construct a new file page manager
     * @param file reference to the parent file
     * @param fileID reference to the file's backend ID
     * @param fileSize initial file size
     * @param pageSize size of pages to use (const)
     * @param pageBackend page backend reference
     */
    PageManager(File& file, uint64_t fileSize, size_t pageSize, PageBackend& pageBackend);

    virtual ~PageManager();
    DELETE_COPY(PageManager)
    DELETE_MOVE(PageManager)

    /** Returns the page size in use */
    [[nodiscard]] size_t GetPageSize() const { return mPageSize; }

    /** Returns the current file size including dirty writes */
    [[nodiscard]] uint64_t GetFileSize(const SharedLock& thisLock) const { return mFileSize; }

    /** Returns a read lock for page data */
    [[nodiscard]] inline SharedLockR GetReadLock() const { return mFile.GetReadLock(); }
    
    /** Returns a priority read lock for page data */
    [[nodiscard]] inline SharedLockRP GetReadPriLock() const { return mFile.GetReadPriLock(); }
    
    /** Returns a write lock for page data */
    inline SharedLockW GetWriteLock() { return mFile.GetWriteLock(); }

    using ScopeLocked = Andromeda::ScopeLocked<PageManager>;
    /** 
     * Tries to lock mScopeMutex, returns a ref that is maybe locked 
     * This protects the pageManager from being deleted while this lock is held
     */
    ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    /** 
     * Reads data from the given page index into buffer
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    void ReadPage(char* buffer, uint64_t index, size_t offset, size_t length, const SharedLock& thisLock);

    /** Writes data to the given page index from buffer
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    void WritePage(const char* buffer, uint64_t index, size_t offset, size_t length, const SharedLockW& thisLock);

    /** 
     * Removes the given page, writing it if dirty
     * @throws BackendException for backend issues
     */
    void EvictPage(uint64_t index, const SharedLockW& thisLock);

    /** 
     * Flushes the given page if dirty, creating the file on the backend if necessary
     * Will also flush any dirty pages sequentially after this one
     * @return the total number of bytes written to the backend
     * @throws BackendException for backend issues
     */
    size_t FlushPage(uint64_t index, const SharedLockW& thisLock);

    /** 
     * Writes back all dirty pages, creating the file on the backend if necessary
     * @throws BackendException for backend issues
     */
    void FlushPages(const SharedLockW& thisLock);

    /**
     * Informs us of the file changing on the backend
     * @param backendSize new size according to the backend
     */
    void RemoteChanged(uint64_t backendSize, const SharedLockW& thisLock);

    /** 
     * Truncate pages according to the given size and inform the backend
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    void Truncate(uint64_t newSize, const SharedLockW& thisLock);

private:

    using UniqueLock = std::unique_lock<std::mutex>;

    /** 
     * Returns the page at the given index and informs cacheMgr - use GetReadLock() first!
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    const Page& GetPageRead(uint64_t index, const SharedLock& thisLock);

    /** 
     * Returns the page at the given index and marks dirty/informs cacheMgr - use GetWriteLock() first! 
     * @param pageSize the desired size of the page for writing
     * @param partial if true, pre-populate the page with backend data
     * @throws BackendException for backend issues
     * @throws CacheManager::MemoryException
     */
    Page& GetPageWrite(uint64_t index, size_t pageSize, bool partial, const SharedLockW& thisLock);

    /** 
     * Calls mCacheMgr->InformPage() on the given page and removes it from mPages if it fails, does not wait for cache space
     * @param canWait if true, maybe wait for cache space (never synchronously)
     * @throws CacheManager::MemoryException if canWait
     */
    void InformNewPageRead(uint64_t index, const Page& page, bool dirty, bool canWait, const UniqueLock& pagesLock);

    /** 
     * Calls mCacheMgr->InformPage() on the given page and removes it from mPages if it fails
     * maybe waits for cache space synchronously, for immediate error-catching
     * @throws CacheManager::MemoryException
     * @throws BackendException for synchronous MemoryException
     */
    void InformNewPageWrite(uint64_t index, const Page& page, bool dirty, const SharedLockW& thisLock);

    /** 
     * Resizes then calls mCacheMgr->InformPage() on the given page and restores size if it fails
     * maybe waits for cache space synchronously, for immediate error-catching
     * @throws CacheManager::MemoryException
     * @throws BackendException for synchronous MemoryException
     */
    void InformResizePage(uint64_t index, Page& page, bool dirty, size_t pageSize, const SharedLockW& thisLock);

    /** 
     * Resizes an existing page to the given size, telling the CacheManager if cacheMgr
     * CALLER MUST LOCK the DataLockW if operating on an existing page!
     * @throws CacheManager::MemoryException if cacheMgr
     */
    void ResizePage(Page& page, size_t pageSize, bool cacheMgr, const SharedLockW* thisLock = nullptr);

    /** Returns true if the page at the given index is pending download */
    bool isFetchPending(uint64_t index, const UniqueLock& pagesLock);

    /** Returns an exception_ptr if the page at the given index failed download else nullptr */
    std::exception_ptr isFetchFailed(uint64_t index, const UniqueLock& pagesLock);

    /** 
     * Returns the read-ahead size to be used for the given VALID (mFileSize) index 
     * Returns 0 if the page exists or the index does not exist on the backend (see mBackendSize)
     */
    size_t GetFetchSize(uint64_t index, const SharedLock& thisLock, const UniqueLock& pagesLock);

    /** Starts a fetch if necessary to prepopulate some pages ahead of the given index (options.readAheadBuffer) */
    void DoAdvanceRead(uint64_t index, const SharedLock& thisLock, const UniqueLock& pagesLock);

    /** Spawns a thread to read some # of pages starting at the given VALID (mBackendSize) index */
    void StartFetch(uint64_t index, size_t readCount, const UniqueLock& pagesLock);

    /** 
     * Reads count# pages from the backend at the given index, adding to the page map
     * Gets its own R thisLock and informs the cacheManager of all new pages
     * Sets mFailedPages[idx] to any BackendException
     */
    void FetchPages(uint64_t index, size_t count) noexcept;

    /** 
     * Removes the given start index from the pending-read list and notifies waiters
     * @param idxOnly if true, adjust the entry start index to be the next index, else remove it
     */
    void RemovePendingFetch(uint64_t index, bool idxOnly, const UniqueLock& pagesLock);

    /** Updates mFetchSize with the given bandwidth measurement - THREAD SAFE */
    void UpdateBandwidth(size_t bytes, const std::chrono::steady_clock::duration& time);

    /** Map of page index to page */
    using PageMap = std::map<uint64_t, Page>;

    /** 
     * Returns a series of **consecutive** dirty pages (total bytes < size_t)
     * @param[in,out] pageIt reference to the iterator to start with - will end as the next index not used
     * @param[out] writeList reference to a list of pages to fill out - guaranteed not empty if pageIt is dirty
     * @return uint64_t the start index of the write list (not valid if writeList is empty!)
     */
    uint64_t GetWriteList(PageMap::iterator& pageIt, PageBackend::PagePtrList& writeList, const SharedLockW& thisLock);

    /** 
     * Writes a series of **consecutive** pages (total < size_t)
     * Also marks each page not dirty and informs the cache manager, and creates the file on the backend if necessary
     * @param index the starting index of the page list
     * @param pages list of pages to flush - may be empty
     * @return the total number of bytes written to the backend
     * @throws BackendException for backend issues
     */
    size_t FlushPageList(uint64_t index, const PageBackend::PagePtrList& pages, const SharedLockW& thisLock);

    /** 
     * Does FlushCreate() in case the file doesn't exist on the backend, then maybe truncates
     *    the file on the backend in case we did a truncate before it existed
     * @throws BackendException for backend issues
     */
    void FlushCreate(const SharedLockW& thisLock);

    mutable Debug mDebug;

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the backend */
    Backend::BackendImpl& mBackend;
    /** Pointer to the cache manager to use */
    CacheManager* mCacheMgr { nullptr };
    /** The size of each page - see description in ConfigOptions */
    const size_t mPageSize;
    /** The current size of the file including dirty extending writes */
    uint64_t mFileSize;

    /** The current read-ahead window (number of pages) (dynamic) - NEVER zero */
    size_t mFetchSize { 1 };
    /** Mutex that protects mFetchSize and mBandwidthHistory */
    std::mutex mFetchSizeMutex;

    /** List of <index,count> pending reads */
    using PendingMap = std::list<std::pair<uint64_t, size_t>>;
    /** 
     * Map of page index to exception thrown when reading 
     * This is simpler but less efficient than storing a map of FetchPairs,
     * but performance is not critical for the rare failure case
     */
    using FailureMap = std::map<uint64_t, std::exception_ptr>;

    /** The index based map of pages */
    PageMap mPages;
    /** Unique set of page ranges being downloaded */
    PendingMap mPendingPages;
    /** Map of failures encountered while downloading pages */
    FailureMap mFailedPages;
    /** Condition variable for waiting for pages */
    std::condition_variable mPagesCV;

    /** Lock for fetch threads to acquire (so destructor can wait) */
    std::shared_mutex mFetchMutex;

    /** List of pages we didn't evict due to sequential writing */
    std::list<uint64_t> mDeferredEvicts;

    /** Shared mutex that is grabbed exclusively when this class is destructed */
    std::shared_mutex mScopeMutex;
    /** Mutex that protects the page maps between concurrent readers (not needed for writers) */
    std::mutex mPagesMutex;

    /** Bandwidth measurement tool for mFetchSize */
    BandwidthMeasure mBandwidth;
    /** Page to/from backend interface */
    PageBackend& mPageBackend;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEMANAGER_H_
