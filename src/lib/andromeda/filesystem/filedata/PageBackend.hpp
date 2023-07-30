
#ifndef LIBA2_PAGEBACKEND_H_
#define LIBA2_PAGEBACKEND_H_

#include <cstdint>
#include <functional>
#include <list>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/SharedMutex.hpp"
#include "andromeda/filesystem/File.hpp"

namespace Andromeda {
namespace Backend { class BackendImpl; }

namespace Filesystem {

namespace Filedata {

class Page;

/** return the size_t min of a (uint64_t and size_t) */
static inline size_t min64st(uint64_t s1, size_t s2) {
    return static_cast<size_t>(std::min(s1, static_cast<uint64_t>(s2)));
}

/** 
 * Handles reading/writing pages from/to the backend
 * THREAD SAFE (FORCES EXTERNAL LOCKS) (use parent File's lock)
 */
class PageBackend
{
public:

    /** 
     * Construct a new file page backend
     * @param file reference to the parent file
     * @param fileID reference to the file's backend ID
     * @param backendSize current size of the file on the backend
     * @param pageSize size of pages to use (const)
     */
    PageBackend(File& file, const std::string& fileID, uint64_t backendSize, size_t pageSize);

    /** 
     * Construct a new file page backend for a file that doesn't exist yet
     * @param file reference to the parent file
     * @param fileID reference to the file's backend ID
     * @param pageSize size of pages to use (const)
     * @param createFunc function to create the file
     * @param uploadFunc function to upload the file
     */
    PageBackend(File& file, const std::string& fileID, size_t pageSize,
      const File::CreateFunc& createFunc, const File::UploadFunc& uploadFunc);
    
    ~PageBackend() = default;
    DELETE_COPY(PageBackend)
    DELETE_MOVE(PageBackend)

    /** Returns true iff the file exists on the backend */
    [[nodiscard]] bool ExistsOnBackend(const SharedLock& thisLock) const { return mBackendExists; }

    /** Returns the file size on the backend */
    [[nodiscard]] uint64_t GetBackendSize(const SharedLock& thisLock) const { return mBackendSize; }

    /** Inform us that the size on the backend has changed */
    void SetBackendSize(uint64_t backendSize, const SharedLockW& thisLock) { mBackendSize = backendSize; }

    /** Callback used to process fetched pages in FetchPages() */
    using PageHandler = std::function<void (const uint64_t, Page&&)>;

    /** 
     * Reads pages from the backend (must mBackendExists!)
     * @param index the page index to start from
     * @param count the number of pages to read
     * @param pageHandler callback for handling constructed pages
     */
    size_t FetchPages(uint64_t index, size_t count, const PageHandler& pageHandler, const SharedLock& thisLock);

    /** Vector of **consecutive** non-null page pointers */
    using PagePtrList = std::vector<Page*>;

    /** 
     * Writes a series of **consecutive** pages (total < size_t)
     * Also creates the file on the backend if necessary (see mBackendExists)
     * @param index the starting index of the page list
     * @param pages list of pages to flush - must NOT be empty
     * @return the total number of bytes written to the backend
     */
    size_t FlushPageList(uint64_t index, const PagePtrList& pages, const SharedLockW& thisLock);

    /** Creates the file on the backend if not mBackendExists and feeds to file.Refresh() */
    void FlushCreate(const SharedLockW& thisLock);

    /** Tell the backend to truncate to the given size, if mBackendExists */
    void Truncate(uint64_t newSize, const SharedLockW& thisLock);

private:

    /** The size of each page - see description in ConfigOptions */
    const size_t mPageSize;
    /** The file size as far as the backend knows */
    uint64_t mBackendSize;

    /** true iff the file has been created on the backend (false if waiting for flush) */
    bool mBackendExists;
    /** Function to create the file if not mBackendExists */
    const File::CreateFunc mCreateFunc;
    /** Function to upload the file if not mBackendExists */
    const File::UploadFunc mUploadFunc;

    /** Reference to the parent file */
    File& mFile;
    /** The file's ID on the backend, valid only if mBackendExists */
    const std::string& mFileID;
    /** Reference to the file's backend */
    Backend::BackendImpl& mBackend;

    mutable Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEBACKEND_H_
