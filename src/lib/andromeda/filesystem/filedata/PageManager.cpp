
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
    mFileSize(file.GetSize()), mBackendSize(file.GetSize()), mDebug("PageManager",this) 
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
const std::byte* PageManager::GetPageRead(const uint64_t index)
{
    mDebug << __func__ << "() " << mFile.GetID() << " (index:" << index << ")"; mDebug.Info();

    UniqueLock pagesLock(mPagesMutex);

    // TODO maybe even if this index exists, always make sure at least the NEXT one is ready?
    // could generalize that with like a minPopulated or something, configurable?

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        mDebug << __func__ << "... return existing page"; mDebug.Info();
        return it->second.mData.data();
    }

    if (!isPending(index, pagesLock))
        StartReadN(index, pagesLock);
    else { mDebug << __func__ << "... already pending"; mDebug.Info(); }
    
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    mDebug << __func__ << "... done waiting for " << index << "!"; mDebug.Info();
    return it->second.mData.data();
}

/*****************************************************/
std::byte* PageManager::GetPageWrite(const uint64_t index, const size_t pwOffset, const size_t pwLength)
{
    mDebug << __func__ << "() " << mFile.GetID() << " (index:" << index 
        << " pwOffset:" << pwOffset << " pwLength:" << pwLength << ")"; mDebug.Info();

    UniqueLock pagesLock(mPagesMutex);

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    mFileSize = std::max(mFileSize, pageStart+pwOffset+pwLength);
    const size_t pageSize { min64st(mFileSize-pageStart, mPageSize) };

    mDebug << __func__ << "... pageSize:" << pageSize 
        << " mFileSize:" << mFileSize; mDebug.Info();

    { PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        ResizePage(it->second, pageSize);

        mDebug << __func__ << "... returning existing page"; mDebug.Info();

        it->second.mDirty = true;
        return it->second.mData.data();
    } }

    bool pending { isPending(index, pagesLock) };
    
    if (!pending && mBackendSize > pageStart &&
        (pwOffset != 0 || pwLength < pageSize)) // partial write of existing page, read first
    {
        mDebug << __func__ << "... partial write, reading single"; mDebug.Info();
        StartRead1(index, pagesLock); // background thread
        pending = true;
    }

    if (pending)
    {
        mDebug << __func__ << "... waiting for pending"; mDebug.Info();

        PageMap::iterator it;
        while ((it = mPages.find(index)) == mPages.end())
            mPagesCV.wait(pagesLock);

        ResizePage(it->second, pageSize);

        mDebug << __func__ << "... returning pended page"; mDebug.Info();

        it->second.mDirty = true;
        return it->second.mData.data();
    }
    else // create a new empty page
    {
        for (uint64_t curIndex { mBackendSize/mPageSize }; curIndex < index; ++curIndex)
        {
            PageMap::iterator it { mPages.find(curIndex) };
            if (it != mPages.end() && it->second.mData.size() != mPageSize)
            {
                mDebug << __func__ << " resizing page index:" << curIndex; mDebug.Info();

                ResizePage(it->second, mPageSize);
            }
        }

        Page& page { mPages.emplace(index, Page(pageSize)).first->second };
        page.mDirty = true; return page.mData.data();
    }
}

/*****************************************************/
bool PageManager::isDirty(const uint64_t index) const
{
    UniqueLock pagesLock(mPagesMutex);

    const PageMap::const_iterator it { mPages.find(index) };
    if (it == mPages.cend()) return false;

    return it->second.mDirty;
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
    const uint64_t lastPage { (mFileSize-1)/mPageSize }; // the last valid page index
    const size_t maxReadCount { min64st(lastPage-index+1, mReadSize) };

    mDebug << __func__ << "(index:" << index << ") fileSize:" << mFileSize
        << " lastPage:" << lastPage << " maxReadCount:" << maxReadCount; mDebug.Info();

    size_t readCount { maxReadCount };
    for (const PageMap::value_type& page : mPages) // stop before next existing
    {
        if (page.first >= index)
        {
            mDebug << __func__ << "... first page at:" << page.first; mDebug.Info();
            readCount = min64st(page.first-index, readCount);
            break; // pages are in order
        }
    }

    for (uint64_t curIndex { index }; curIndex < index+readCount; ++curIndex) // stop before next pending
    {
        if (isPending(curIndex, pagesLock))
        {
            mDebug << __func__ << "... pending page at:" << curIndex; mDebug.Info();
            readCount = curIndex - index;
            break; // pages are in order
        }
    }

    mDebug << __func__ << "... return:"  << readCount; mDebug.Info();
    return readCount;
}

/*****************************************************/
void PageManager::ReadPages(const uint64_t index, const size_t count, PageManager::SharedLockR classLock)
{
    SharedLockR readersLock(mReadersMutex);
    SemaphorLock threadSem(sBackendSem);

    SharedLockR dataLock; // empty
    // if count is 1, waiter's dataLock has us covered
    if (count > 1) dataLock = SharedLockR(mDataMutex);

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const size_t readSize { pageStart > mBackendSize ? 0 : 
        min64st(mBackendSize-pageStart, mPageSize*count) }; // length of data to fetch

    mDebug << __func__ << "... threads:" << sBackendSem.get_count() << " index:" << index 
        << " count:" << count << " pageStart:" << pageStart << " readSize:" << readSize; mDebug.Info();

    uint64_t curIndex { index };
    std::unique_ptr<Page> curPage;

    if (readSize) mBackend.ReadFile(mFile.GetID(), pageStart, readSize, 
        [&](const size_t roffset, const char* rbuf, const size_t rlength)->void
    {
        // this is basically the same as the File::WriteBytes() algorithm
        for (uint64_t rbyte { roffset }; rbyte < roffset+rlength; )
        {
            if (!curPage) 
            {
                const uint64_t curPageStart { curIndex*mPageSize };
                const size_t pageSize { min64st(mFileSize-curPageStart, mPageSize) };
                curPage = std::make_unique<Page>(pageSize);

                if (mBackendSize-curPageStart < pageSize)
                {
                    mDebug << __func__ << "... partial page:" << curIndex; mDebug.Info();
                    std::memset(curPage->mData.data(), 0, pageSize);
                }
            }

            const uint64_t rindex { rbyte / mPageSize }; // page index for this data
            const size_t pwOffset { static_cast<size_t>(rbyte - rindex*mPageSize) }; // offset within the page
            const size_t pwLength { min64st(rlength+roffset-rbyte, mPageSize-pwOffset) }; // length within the page

            if (rindex == curIndex-index)
            {
                char* pageBuf { reinterpret_cast<char*>(curPage->mData.data()) };
                memcpy(pageBuf+pwOffset, rbuf, pwLength);

                if (pwOffset+pwLength == curPage->mData.size()) // page is done
                {
                    UniqueLock pagesLock(mPagesMutex);

                    mPages.emplace(curIndex, std::move(*curPage));
                    RemovePending(curIndex, pagesLock); 
                    curPage.reset(); ++curIndex;
                }
            }
            else { mDebug << __func__ << "... old read, ignoring"; mDebug.Info(); }

            rbuf += pwLength; rbyte += pwLength;
        }
    });

    for (; curIndex < index+count; curIndex++) // maybe reading beyond mBackendSize
    {
        mDebug << __func__ << " creating empty page:" << curIndex; mDebug.Info();

        UniqueLock pagesLock(mPagesMutex);

        const uint64_t curPageStart { curIndex*mPageSize };
        const size_t pageSize { min64st(mFileSize-curPageStart, mPageSize) };
        
        Page& page { mPages.emplace(curIndex, Page(pageSize)).first->second };
        std::memset(page.mData.data(), 0, pageSize);
        RemovePending(curIndex, pagesLock);
    }

    mDebug << __func__ << "... thread returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::WritePages(PageManager::PageRefMap& pages, PageManager::SharedLockR& dataLock)
{
    SemaphorLock threadSem(sBackendSem);

    mDebug << __func__ << "(pages:" << pages.size() << ")"; mDebug.Info();

    uint64_t totalSize { 0 };
    for (const PageRefMap::value_type& it : pages)
        totalSize += it.second.mData.size();

    Bytes buf(totalSize);
    uint64_t curOffset { 0 }; 
    for (const PageRefMap::value_type& it : pages)
    {
        const Page::Data& pageData { it.second.mData };
        const auto iCurOffset { static_cast<Page::Data::iterator::difference_type>(curOffset) };

        std::copy(pageData.cbegin(), pageData.cend(), buf.begin()+iCurOffset);
        curOffset += pageData.size();
    }

    uint64_t writeStart { pages.begin()->first*mPageSize };
    std::string data(reinterpret_cast<const char*>(buf.data()), totalSize);

    mDebug << __func__ << "... WRITING " << data.size() << " to " << writeStart; mDebug.Info();

    mBackend.WriteFile(mFile.GetID(), writeStart, data);

    for (PageRefMap::value_type& it : pages)
        it.second.mDirty = false;

    mBackendSize = std::max(mBackendSize, writeStart+totalSize);
}

/*****************************************************/
void PageManager::FlushPages(bool nothrow)
{
    mDebug << __func__ << "()"; mDebug.Info();

    std::list<PageRefMap> writelist;

    SharedLockR dataLock(mDataMutex);

    { // lock scope
        UniqueLock pagesLock(mPagesMutex);

        mDebug << __func__ << "... have locks"; mDebug.Info();

        decltype(writelist)::iterator curMap = writelist.end();
        for (decltype(mPages)::value_type& it : mPages)
        {
            if (it.second.mDirty)
            {
                if (curMap == writelist.end())
                {
                    mDebug << __func__ << "... new write run!"; mDebug.Info();
                    writelist.emplace_front(); curMap = writelist.begin();
                }

                curMap->emplace(it.first, it.second);
            }
            else if (curMap != writelist.end())
                curMap = writelist.end(); // end run
        }
    }

    while (!writelist.empty())
    {
        auto writeFunc { [&]()->void { WritePages(writelist.back(), dataLock); } };

        if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const BaseException& e) { 
            mDebug << __func__ << "... Ignoring Error: " << e.what(); mDebug.Error(); }

        writelist.pop_back();
    }

    mDebug << __func__ << "... returning!"; mDebug.Info();
}

/*****************************************************/
bool PageManager::EvictPage(const uint64_t index)
{
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);
    
    mDebug << __func__ << "... have locks"; mDebug.Info();

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        Page& page { it->second };
        if (page.mDirty)
        {
            mDebug << __func__ << "... page is dirty, writing"; mDebug.Info();

            std::string data(reinterpret_cast<const char*>(page.mData.data()), page.mData.size());
            mBackend.WriteFile(mFile.GetID(), it->first*mPageSize, data);
            // TODO !! return bool based on success/fail (don't throw)
        }

        mPages.erase(it);

        mDebug << __func__ << "... page removed, returning"; mDebug.Info();
    }

    return true;
}

/*****************************************************/
void PageManager::RemoteChanged(const uint64_t backendSize)
{    
    // TODO add ability to interrupt in-progress reads (same as destructor)
    // since the download is streamed you should be able to just set a flag to have it reply false to httplib
    // then catch the httplib::Canceled and ignore it and return if that flag is set - will help error handling later

    mDebug << __func__ << "(newSize:" << backendSize << ")" 
        << " oldSize:" << mBackendSize << ")"; mDebug.Info();

    SharedLockW readersLock(mReadersMutex); // stop downloads

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);

    mDebug << __func__ << "... have locks"; mDebug.Info();

    uint64_t maxDirty { 0 }; // evict all pages
    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        Page& page { it->second };
        if (!page.mDirty)
        {
            it = mPages.erase(it);
        }
        else
        {
            uint64_t pageMax { it->first*mPageSize + page.mData.size() };
            maxDirty = std::max(maxDirty, pageMax);
            ++it; // move to next page
        }
    }

    mBackendSize = backendSize;
    mFileSize = std::max(backendSize, maxDirty);
}

/*****************************************************/
void PageManager::Truncate(const uint64_t newSize)
{
    mDebug << __func__ << "(newSize:" << newSize << ")"; mDebug.Info();

    SharedLockW readersLock(mReadersMutex); // stop downloads

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);

    mDebug << __func__ << "... have locks"; mDebug.Info();

    mBackend.TruncateFile(mFile.GetID(), newSize); 

    mBackendSize = newSize;
    mFileSize = newSize;

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/mPageSize) // remove past end
        {
            mDebug << __func__ << "... erase page:" << it->first; mDebug.Info();

            it = mPages.erase(it); 

            mDebug << __func__ << "... page removed!" << it->first; mDebug.Info();
        }
        else if (it->first == (newSize-1)/mPageSize) // resize last page
        {
            uint64_t pageSize { newSize - it->first*mPageSize };
            mDebug << __func__<< "... resize page:" << it->first 
                << " size:" << pageSize; mDebug.Info();

            ResizePage(it->second, pageSize);

            mDebug << __func__ << "... page resized!"; mDebug.Info();
        }
        else ++it;
    }
}

/*****************************************************/
void PageManager::ResizePage(Page& page, uint64_t pageSize)
{
    const uint64_t oldSize { page.mData.size() };
    page.mData.resize(pageSize);

    if (pageSize > oldSize) std::memset(
        page.mData.data()+oldSize, 0, pageSize-oldSize);
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda

