
#ifndef LIBA2_PAGE_H_
#define LIBA2_PAGE_H_

#include <vector>

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

typedef std::vector<std::byte> Bytes;

/** return the size_t min of a (uint64_t and size_t) */
static inline size_t min64st(uint64_t s1, size_t s2)
{
    return static_cast<size_t>(std::min(s1, static_cast<uint64_t>(s2)));
}

/** A file data page */
struct Page
{
    explicit Page(size_t pageSize) : mData(pageSize) { }
    typedef Bytes Data; Data mData;
    bool mDirty { false };
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGE_H_
