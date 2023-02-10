
#ifndef LIBA2_PAGEBACKEND_H_
#define LIBA2_PAGEBACKEND_H_

#include <functional>
#include <list>

#include "Page.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Backend { class BackendImpl; }

namespace Filesystem {
class File;

namespace Filedata {

/** 
 * Handles reading/writing pages from/to the backend 
 * WARNING - NOT thread safe! Use external R/W locking
 */
class PageBackend
{
public:

    /** 
     * Construct a new file page backend
     * @param file reference to the parent file
     * @param pageSize size of pages to use (const)
     * @param backendSize current size of the file on the backend
     * @param backendExists true if the file exists on the backend
     */
    PageBackend(File& file, const size_t pageSize, uint64_t backendSize, bool backendExists);

    /** Returns true iff the file exists on the backend */
    bool ExistsOnBackend() const { return mBackendExists; }

    /** Returns the file size on the backend */
    uint64_t GetBackendSize() const { return mBackendSize; }

    /** Inform us that the size on the backend has changed */
    void SetBackendSize(uint64_t backendSize) { mBackendSize = backendSize; }

    /** Callback used to process fetched pages in FetchPages() */
    typedef std::function<void(const uint64_t pageIndex, const uint64_t pageStart, const size_t pageSize, Page& page)> PageHandler;

    /** 
     * Reads pages from the backend
     * @param index the page index to start from
     * @param count the number of pages to read
     * @param pageHandler callback for handling constructed pages
     */
    size_t FetchPages(const uint64_t index, const size_t count, const PageHandler& pageHandler);

    /** List of **consecutive** page pointers */
    typedef std::list<Page*> PagePtrList;

    /** 
     * Writes a series of **consecutive** pages (total < size_t)
     * Also creates the file on the backend if necessary (see mBackendExists)
     * @param index the starting index of the page list
     * @param pages list of pages to flush - must NOT be empty
     * @return the total number of bytes written to the backend
     */
    size_t FlushPageList(const uint64_t index, const PagePtrList& pages);

    /** Creates the file on the backend if not
      * mBackendExists and feeds to file.Refresh() */
    void FlushCreate();

    /** Tell the backend to truncate to the given size, if mBackendExists */
    void Truncate(const uint64_t newSize);

private:

    /** The size of each page - see description in ConfigOptions */
    const size_t mPageSize;
    /** The file size as far as the backend knows */
    uint64_t mBackendSize;
    /** true iff the file has been created on the backend (false if waiting for flush) */
    bool mBackendExists;

    /** Reference to the parent file */
    File& mFile;
    /** Reference to the file's backend */
    Backend::BackendImpl& mBackend;

    Debug mDebug;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGEBACKEND_H_
