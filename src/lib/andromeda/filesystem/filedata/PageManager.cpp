
#include <cstring>
#include <nlohmann/json.hpp>

#include "PageManager.hpp"
#include "andromeda/Semaphor.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;

// globally limit the maximum number of concurrent background ops
static Andromeda::Semaphor sBackendSem { 4 }; // TODO configurable later?

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
PageManager::PageManager(File& file, BackendImpl& backend, const size_t pageSize) : 
    mFile(file), mBackend(backend), mPageSize(pageSize), 
    mBackendSize(file.GetSize()), mDebug("PageManager") 
{ 
    mDebug << __func__ << "(file: " << file.GetName() << " pageSize:" << pageSize << ")"; mDebug.Info();
}

/*****************************************************/
PageManager::~PageManager()
{
    mDebug << __func__ << "() waiting for threads"; mDebug.Info();

    // TODO interrupt existing threads? could set stream failbit to stop HTTPRunner
    SharedLockW classLock(mClassMutex);

    mDebug << __func__ << "... returning!"; mDebug.Info();
}

/*****************************************************/
const PageReader PageManager::GetPageReader(const uint64_t index)
{
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    UniqueLock pagesLock(mPagesMutex);

    // TODO maybe even if this index exists, always make sure at least the NEXT one is ready?
    // could generalize that with like a minPopulated or something, configurable?

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        mDebug << __func__ << "... return existing page"; mDebug.Info();
        return PageReader(*(it->second));
    }

    if (!isPending(index, pagesLock))
        StartReadN(index, pagesLock);
    else { mDebug << __func__ << "... already pending"; mDebug.Info(); }
    
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    mDebug << __func__ << "... done waiting for " << index << "!"; mDebug.Info();
    return PageReader(*(it->second));
}

/*****************************************************/
PageWriter PageManager::GetPageWriter(const uint64_t index, const size_t offset, const size_t length)
{
    mDebug << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info();

    UniqueLock pagesLock(mPagesMutex);

    const uint64_t pageStart { index*mPageSize };
    const uint64_t pageMax { (mBackendSize < pageStart) 
        ? offset+length : std::max(mBackendSize-pageStart, offset+length) };

    const size_t pageSize { min64st(pageMax, mPageSize) };
    mDebug << __func__ << "... pageSize:" << pageSize; mDebug.Info();

    PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        mDebug << __func__ << "... locking existing page"; mDebug.Info();

        PageWriter page(*(it->second));
        if (page->size() < offset+length) 
            page->resize(offset+length);

        mDebug << __func__ << "... returning existing page"; mDebug.Info();

        return page;
    }

    bool pending { isPending(index, pagesLock) };
    if (pending || (offset || length < pageSize)) // partial write
    {
        if (!pending)
        {
            mDebug << __func__ << "... partial write, reading single"; mDebug.Info();
            StartRead1(index, pagesLock); // background thread 
        }

        mDebug << __func__ << "... waiting for pending"; mDebug.Info();

        while ((it = mPages.find(index)) == mPages.end())
            mPagesCV.wait(pagesLock);

        mDebug << __func__ << "... locking pended page"; mDebug.Info();

        PageWriter page(*(it->second));
        if (page->size() < offset+length) 
            page->resize(offset+length);

        mDebug << __func__ << "... returning pended page"; mDebug.Info();

        return page;
    }
    else // just create an empty page
    {
        PagePtr page { std::make_unique<Page>(pageSize) };
        mDebug << __func__ << " created page! returning"; mDebug.Info();

        PageWriter ret(*page); // lock page
        mPages.emplace(index, std::move(page));
        return ret;
    }
}

/*****************************************************/
bool PageManager::isDirty(const uint64_t index)
{
    UniqueLock pagesLock(mPagesMutex);
    return isDirty(index, pagesLock);
}

/*****************************************************/
bool PageManager::isDirty(const uint64_t index, PageManager::UniqueLock& pagesLock)
{
    const PageMap::const_iterator it { mPages.find(index) };
    if (it == mPages.cend()) return false;

    return it->second->mDirty;
}

/*****************************************************/
bool PageManager::isPending(const uint64_t index, PageManager::UniqueLock& pagesLock)
{
    for (const decltype(mPendingPages)::value_type& pend : mPendingPages)
        if (index >= pend.first && index < pend.first + pend.second) return true;
    return false;
}

/*****************************************************/
void PageManager::RemovePending(const uint64_t index, PageManager::UniqueLock& pagesLock)
{
    for (PendingMap::iterator pend { mPendingPages.begin() }; 
        pend != mPendingPages.end(); ++pend)
    {
        if (index == pend->first)
        {
            ++pend->first; // index
            --pend->second; // count
            if (!pend->second) 
                mPendingPages.erase(pend);
            break;
        }
    }

    mPagesCV.notify_all();
}

/*****************************************************/
void PageManager::StartRead1(const uint64_t index, PageManager::UniqueLock& pagesLock)
{
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    mPendingPages.emplace_back(index, 1);

    SharedLockR classLock(mClassMutex); // grab before creating
    std::thread(&PageManager::ReadPages, this, index, 1, 
        std::move(classLock)).detach();
}

/*****************************************************/
void PageManager::StartReadN(const uint64_t index, PageManager::UniqueLock& pagesLock)
{
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    size_t readCount { GetReadSize(index, pagesLock) };

    mDebug << __func__ << "... add pending pages count:" << readCount; mDebug.Info();

    mPendingPages.emplace_back(index, readCount);

    SharedLockR classLock(mClassMutex); // grab before creating
    std::thread(&PageManager::ReadPages, this, index, readCount, 
        std::move(classLock)).detach();
}

/*****************************************************/
uint64_t PageManager::GetReadSize(const uint64_t index, PageManager::UniqueLock& pagesLock)
{
    const uint64_t lastPage { (mBackendSize-1)/mPageSize };
    const size_t maxReadCount { min64st(lastPage-index+1, mReadSize) };

    mDebug << __func__ << "(index:" << index << ") backendSize:" << mBackendSize
        << " lastPage:" << lastPage << " maxReadCount:" << maxReadCount; mDebug.Info();

    size_t readCount { maxReadCount };
    for (const PageMap::value_type& page : mPages) // check existing
    {
        if (page.first >= index)
        {
            mDebug << __func__ << "... first page at:" << page.first; mDebug.Info();
            readCount = min64st(page.first-index, readCount);
            break; // pages are in order
        }
    }

    for (uint64_t idx { index }; idx < index+readCount; ++idx) // check pending
    {
        if (isPending(idx, pagesLock))
        {
            mDebug << __func__ << "... pending page at:" << idx; mDebug.Info();
            readCount = idx - index;
            break; // pages are in order
        }
    }

    mDebug << __func__ << "... return:"  << readCount; mDebug.Info();
    return readCount;
}

/*****************************************************/
void PageManager::ReadPages(const uint64_t index, const size_t count, PageManager::SharedLockR classLock)
{
    SemaphorLock threadSem(sBackendSem);
    SharedLockR readersLock(mReadersMutex);

    const uint64_t offset { index*mPageSize };
    const size_t readsize { min64st(mBackendSize-offset, mPageSize*count) };

    mDebug << __func__ << "... threads:" << sBackendSem.get_count() << " index:" << index 
        << " count:" << count << " offset:" << offset << " readsize:" << readsize; mDebug.Info();

    uint64_t curIndex { index };
    PagePtr curPage;

    mBackend.ReadFile(mFile.GetID(), offset, readsize, 
        [&](const size_t roffset, const char* rbuf, const size_t rlength)->void
    {
        // this is basically the same as the File::WriteBytes() algorithm
        for (uint64_t rbyte { roffset }; rbyte < roffset+rlength; )
        {
            if (!curPage) 
            {
                const size_t pageSize { min64st(mBackendSize - curIndex*mPageSize, mPageSize) }; // TODO !! mBackendSize lock?
                curPage = std::make_unique<Page>(pageSize);
            }

            const uint64_t rindex { rbyte / mPageSize };
            const size_t pOffset { static_cast<size_t>(rbyte - rindex*mPageSize) };
            const size_t pLength { min64st(rlength+roffset-rbyte, mPageSize-pOffset) };

            if (rindex == curIndex-index)
            {
                char* pageBuf { reinterpret_cast<char*>(curPage->mData.data()) };
                memcpy(pageBuf+pOffset, rbuf, pLength);

                if (pOffset+pLength == curPage->mData.size()) // page is done
                {
                    UniqueLock pagesLock(mPagesMutex);

                    mPages.emplace(curIndex, std::move(curPage));
                    RemovePending(curIndex, pagesLock); curIndex++;
                }
            }
            else { mDebug << __func__ << "... old read, ignoring"; mDebug.Info(); }

            rbuf += pLength; rbyte += pLength;
        }
    });

    mDebug << __func__ << "... thread returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::WritePages(PageManager::LockedPageMap& pages)
{
    SemaphorLock threadSem(sBackendSem);

    mDebug << __func__ << "(pages:" << pages.size() << ")"; mDebug.Info();

    uint64_t totalSize { 0 };
    for (const LockedPageMap::value_type& it : pages)
        totalSize += it.second->size();

    mDebug << __func__ << "... totalSize:" << totalSize << ")"; mDebug.Info();

    Bytes buf(totalSize);
    uint64_t curOffset { 0 }; 
    for (const LockedPageMap::value_type& it : pages)
    {
        const PageReader& page { it.second };
        const auto iCurOffset { static_cast<Page::Data::iterator::difference_type>(curOffset) };
        std::copy(page->cbegin(), page->cend(), buf.begin()+iCurOffset);
        curOffset += page->size();
    }

    uint64_t writeStart { pages.begin()->first };
    std::string data(reinterpret_cast<const char*>(buf.data()), totalSize);
    mBackend.WriteFile(mFile.GetID(), writeStart, data);

    for (LockedPageMap::value_type& it : pages)
        it.second.ResetDirty();

    mBackendSize = std::max(mBackendSize, writeStart+totalSize); // TODO !! mBackendSize lock
}

/*****************************************************/
bool PageManager::EvictPage(const uint64_t index)
{
    UniqueLock pagesLock(mPagesMutex);

    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        mDebug << __func__ << "... page exists, waiting"; mDebug.Info();

        Page& page { *(it->second) };
        SharedLockW pageLock(page.mMutex);

        if (page.mDirty)
        {
            mDebug << __func__ << "... page is dirty, writing"; mDebug.Info();

            std::string data(reinterpret_cast<const char*>(page.mData.data()), page.mData.size());
            mBackend.WriteFile(mFile.GetID(), it->first, data);
            // TODO !! return bool based on success/fail (don't throw)
        }

        mPages.erase(it);

        mDebug << __func__ << "... page removed, returning"; mDebug.Info();
    }

    return true;
}

/*****************************************************/
uint64_t PageManager::EvictReadPages()
{
    // TODO add ability to interrupt in-progress reads (same as destructor)
    // since the download is streamed you should be able to just set a flag to have it reply false to httplib
    // then catch the httplib::Canceled and ignore it and return if that flag is set - will help error handling later

    SharedLockW readersLock(mReadersMutex); // stop downloads
    UniqueLock pagesLock(mPagesMutex);

    uint64_t maxDirty { 0 };
    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        Page& page { *(it->second) };
        if (!page.mDirty)
        {
            SharedLockW pageLock(page.mMutex);
            it = mPages.erase(it);
        }
        else
        {
            uint64_t pageMax { it->first + page.mData.size() };
            maxDirty = std::max(maxDirty, pageMax);
            ++it; // move to next page
        }
    }

    return maxDirty;
}

/*****************************************************/
void PageManager::FlushPages(bool nothrow)
{
    mDebug << __func__ << "()"; mDebug.Info();

    typedef std::list<LockedPageMap> WriteList;
    WriteList writelist;

    { // lock scope
        UniqueLock pagesLock(mPagesMutex);

        WriteList::iterator curMap = writelist.end();
        for (decltype(mPages)::value_type& it : mPages)
        {
            if (it.second->mDirty)
            {
                if (curMap == writelist.end())
                {
                    mDebug << __func__ << "... new write run!"; mDebug.Info();
                    writelist.emplace_front(); curMap = writelist.begin();
                }

                mDebug << __func__ << "... dirty page:" << it.first; mDebug.Info();
                curMap->emplace(it.first, PageReader(*(it.second)));
                mDebug << __func__ << "... page added"; mDebug.Info();
            }
            else if (curMap != writelist.end())
                curMap = writelist.end();
        }
    }

    
    while (!writelist.empty())
    {
        auto writeFunc { [&]()->void { WritePages(writelist.back()); } };

        if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const BaseException& e) { 
            mDebug << __func__ << "... Ignoring Error: " << e.what(); mDebug.Error(); }

        writelist.pop_back();
    }
}

/*****************************************************/
uint64_t PageManager::RemoteChanged(const uint64_t backendSize, bool reset)
{
    mDebug << __func__ << "(newSize:" << backendSize << ")" 
        << " oldSize:" << mBackendSize << ")"; mDebug.Info();

    mBackendSize = backendSize; // TODO !! mBackendSize lock? or just wait for big single lock

    return reset ? EvictReadPages() : 0;
}

/*****************************************************/
void PageManager::Truncate(const uint64_t newSize)
{
    UniqueLock pagesLock(mPagesMutex);

    mDebug << __func__ << "(newSize:" << newSize << ")"; mDebug.Info();

    mBackend.TruncateFile(mFile.GetID(), newSize); 

    mBackendSize = newSize; // TODO !! backendSize locking? r just wait for big single lock

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/mPageSize) // remove past end
        {
            mDebug << __func__ << "... erase page:" << it->first; mDebug.Info();

            SharedLockW pageLock(it->second->mMutex);
            it = mPages.erase(it); 

            mDebug << __func__ << "... page removed!" << it->first; mDebug.Info();
        }
        else if (it->first == (newSize-1)/mPageSize) // resize last page
        {
            uint64_t pageSize { newSize - it->first*mPageSize };
            mDebug << __func__<< "... resize page:" << it->first 
                << " size:" << pageSize; mDebug.Info();

            SharedLockW pageLock(it->second->mMutex);
            it->second->mData.resize(pageSize);

            mDebug << __func__ << "... page resized!"; mDebug.Info();
        }
        else it++;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

