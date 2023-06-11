
#ifndef LIBA2_PAGE_H_
#define LIBA2_PAGE_H_

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

class CachingAllocator;

/** A file data page (manages memory pages) */
class Page
{
public:

    /** Construct a page with the given size in bytes and allocator */
    explicit Page(size_t pageSize, CachingAllocator& memAlloc);

    Page(Page&&);
    virtual ~Page();

    /** Return a pointer to the data buffer */
    inline char* data() { return mData; }
    inline const char* data() const { return mData; }
    /** Return the size of this page in bytes */
    inline size_t size() const { return mBytes; }
    /** Returns the memory usage of this page in bytes */
    size_t capacity() const;

    /** Return true if the page is dirty (un-flushed data) */
    inline bool isDirty() const { return mDirty; }
    /** Set whether or not this page is dirty */
    inline void setDirty(bool dirty = true){ mDirty = dirty; }

    /** Resizes to the given # of bytes, possibly re-allocating */
    void resize(size_t bytes);

    // do not allow copying
    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;
    Page& operator=(Page&&) = delete;

private:

    // could use a std::vector with an Allocator instead of a raw buffer
    // but vector lies about its actual memory usage... size vs. capacity

    /** Allocator to use for memory pages */
    CachingAllocator& mAlloc;
    /** Size of this page in bytes */
    size_t mBytes;
    /** Number of memory pages allocated */
    size_t mPages;
    /** Pointer to allocated memory */
    char* mData;
    /** true if the page has dirty (un-flushed) data */
    bool mDirty { false };
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGE_H_
