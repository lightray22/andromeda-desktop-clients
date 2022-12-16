
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

/** A file data page */
struct Page
{
    explicit Page(size_t pageSize) : data(pageSize) { }

    typedef std::vector<std::byte> Data; Data data;
    
    bool dirty { false };
    std::shared_mutex mutex;
};

typedef std::shared_ptr<Page> SharedPage;

/** A shared-locked read-only page reference */
class PageReader
{
public:
    explicit PageReader(SharedPage& page) : 
        mPage(page), mLock(page->mutex) { }

    const Page::Data& GetData() const { return mPage->data; }
    const Page::Data& operator*() const { return mPage->data; }
    const Page::Data* operator->() const { return &mPage->data; }

private:
    SharedPage mPage;
    std::shared_lock<std::shared_mutex> mLock;
};

/** An exclusive-locked writable page reference */
class PageWriter
{
public:
    explicit PageWriter(SharedPage& page) : 
        mPage(page), mLock(page->mutex) { }

    Page::Data& GetData() { return mPage->data; }
    Page::Data& operator*() { return mPage->data; }
    Page::Data* operator->() { return &mPage->data; }

private:
    SharedPage mPage;
    std::unique_lock<std::shared_mutex> mLock;
};

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PAGE_H_
