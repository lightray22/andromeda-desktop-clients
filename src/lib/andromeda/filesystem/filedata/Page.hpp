
#ifndef LIBA2_PAGE_H_
#define LIBA2_PAGE_H_

#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
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

    typedef Bytes Data; 

    Data mData;
    bool mDirty { false };
    std::shared_mutex mMutex;
};

/** A shared-locked read-only page reference */
class PageReader
{
public:
    explicit PageReader(Page& page) : 
        mPage(page), mLock(page.mMutex) { }

    const Page::Data& operator*() const { return mPage.mData; }
    const Page::Data* operator->() const { return &mPage.mData; }

    /** Reset the page's dirty flag to false */
    void ResetDirty() { mPage.mDirty = false; }

private:
    Page& mPage;
    std::shared_lock<std::shared_mutex> mLock;
};

/** An exclusive-locked writable page reference */
class PageWriter
{
public:
    explicit PageWriter(Page& page) : 
        mPage(page), mLock(page.mMutex) { page.mDirty = true; }

    Page::Data& operator*() { return mPage.mData; }
    Page::Data* operator->() { return &mPage.mData; }
    const Page::Data& operator*() const { return mPage.mData; }
    const Page::Data* operator->() const { return &mPage.mData; }

private:
    Page& mPage;
    std::unique_lock<std::shared_mutex> mLock;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGE_H_
