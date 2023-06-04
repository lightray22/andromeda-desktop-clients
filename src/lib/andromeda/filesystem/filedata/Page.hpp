
#ifndef LIBA2_PAGE_H_
#define LIBA2_PAGE_H_

#include <vector>

#include "CachingAllocator.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/** return the size_t min of a (uint64_t and size_t) */
static inline size_t min64st(uint64_t s1, size_t s2)
{
    return static_cast<size_t>(std::min(s1, static_cast<uint64_t>(s2)));
}

/** A file data page */
struct Page
{
    explicit Page(size_t pageSize, CachingAllocatorT<char>& pageAlloc) :
        mData(pageSize, pageAlloc) { }

    /** byte array of page data */
    std::vector<char, CachingAllocatorT<char>> mData;

    /** true if the page has dirty (un-flushed) data */
    bool mDirty { false };

    // accessors for convenience
    char* data() { return mData.data(); }
    const char* data() const { return mData.data(); }
    size_t size() const { return mData.size(); }

    // allow moving but not copying
    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;
    Page(Page&&) = default;
    Page& operator=(Page&&) = default;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGE_H_
