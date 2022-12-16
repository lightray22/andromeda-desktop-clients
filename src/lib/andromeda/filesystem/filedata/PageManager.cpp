
#include "PageManager.hpp"
#include "andromeda/Semaphor.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;

// globally limit the maximum number of background threads
static Andromeda::Semaphor sThreadSem { 4 }; // TODO configurable later?

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/** return the size_t min of a (uint64_t and size_t) */
size_t min64st(uint64_t s1, size_t s2)
{
    return static_cast<size_t>(std::min(s1, static_cast<uint64_t>(s2)));
}

/*****************************************************/
PageManager::PageManager(File& file, BackendImpl& backend, size_t pageSize) : 
    mFile(file), mBackend(backend), mPageSize(pageSize), mDebug("PageManager") 
{ 
    mDebug << __func__ << "(file: " << file.GetName() << " pageSize:" << pageSize << ")"; mDebug.Info();
}

/*****************************************************/
PageManager::~PageManager()
{
    mDebug << __func__ << "() waiting for threads"; mDebug.Info();
    // TODO interrupt existing threads? could set stream failbit to stop HTTPRunner
    std::unique_lock<std::shared_mutex> threads_lock(mThreadsMutex);
    mDebug << __func__ << "... returning!"; mDebug.Info();
}

/*****************************************************/
const PageReader PageManager::GetPageReader(const uint64_t index)
{
    UniqueLock pages_lock(mPagesMutex);

    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    return std::move(PageReader(GetPage(index, pages_lock)));
}

/*****************************************************/
PageWriter PageManager::GetPageWriter(const uint64_t index)
{
    UniqueLock pages_lock(mPagesMutex);
    
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    return std::move(PageWriter(GetPage(index, pages_lock)));
}

/*****************************************************/
void PageManager::DeletePage(const uint64_t index)
{
    UniqueLock pages_lock(mPagesMutex);

    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    // TODO check if page is dirty and writeback

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) ErasePage(it);
}

/*****************************************************/
void PageManager::Truncate(const uint64_t newSize)
{
    UniqueLock pages_lock(mPagesMutex);

    mDebug << __func__ << "(newSize:" << newSize << ")"; mDebug.Info();

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/mPageSize)
        {
            it = ErasePage(it); // remove past end
        }
        else ++it; // move to next page
    }
}

/*****************************************************/
SharedPage& PageManager::GetPage(const uint64_t index, PageManager::UniqueLock& pages_lock)
{
    mDebug << __func__ << "(index:" << index << " readAhead:" << mReadAhead << ")"; mDebug.Info();

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        mDebug << __func__ << "... return existing page"; mDebug.Info();
        return it->second;
    }

    if (mPendingPages.find(index) == mPendingPages.end())
    {
        mDebug << __func__ << "... add pending page"; mDebug.Info();
        mPendingPages.emplace(index);

        // grab thread locks before creating it
        SemaphorLock thread_count(sThreadSem);
        SharedLock threads_lock(mThreadsMutex);

        std::thread(&PageManager::FetchPage, this, index, 
            std::move(thread_count), std::move(threads_lock)).detach();
    }
    else { mDebug << __func__ << "... page already pending"; mDebug.Info(); }

    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pages_lock);

    mDebug << __func__ << "... done waiting!"; mDebug.Info();
    return it->second;
}

/*****************************************************/
void PageManager::FetchPage(const uint64_t index, SemaphorLock thread_count, PageManager::SharedLock threads_lock)
{
    const uint64_t offset { index*mPageSize };
    const size_t readsize { min64st(mFile.GetSize()-offset, mPageSize) };

    mDebug << __func__ << "... threads:" << sThreadSem.get_count() 
        << " index:" << index << " offset:" << offset << " readsize:" << readsize; mDebug.Info();

    // TODO implement download streaming and early signalling

    bool hasData { readsize > 0 && offset < mFile.GetBackendSize() };
    const std::string data(hasData ? mBackend.ReadFile(mFile.GetID(), offset, readsize) : "");

    {
        UniqueLock pages_lock(mPagesMutex);

        // for the first page we keep it minimal to save memory on small files
        // for subsequent mPages we allocate the full size ahead of time for speed
        const size_t pageSize { (index == 0) ? readsize : mPageSize };

        PageMap::iterator it { mPages.emplace(index, 
            std::make_shared<Page>(pageSize)).first };

        char* buf { reinterpret_cast<char*>(it->second->data.data()) };
        std::copy(data.cbegin(), data.cend(), buf);

        mPendingPages.erase(index);
    }

    mPagesCV.notify_all();
}

/*****************************************************/
PageManager::PageMap::iterator PageManager::ErasePage(PageMap::iterator& it)
{
    // mPagesMutex must already be locked!
    mDebug << __func__ << "()"; mDebug.Info();

    // Get a shared_ptr to the page so it stays valid after removing
    // locks cannot be moved which is why we must use shared_ptr
    SharedPage page(it->second);
    // then remove it from the map to prevent further use
    PageMap::iterator retIt { mPages.erase(it) };

    mDebug << __func__ << "... waiting for exclusive lock"; mDebug.Info();

    // then get an exclusive lock to make sure everyone's done
    PageWriter pageW(page);

    mDebug << __func__ << "... got exclusive lock, returning"; mDebug.Info();

    return retIt;
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
