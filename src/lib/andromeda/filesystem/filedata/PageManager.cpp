
#include <cassert>
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
PageManager::PageManager(File& file, BackendImpl& backend, const uint64_t fileSize, const size_t pageSize) : 
    mFile(file), mBackend(backend), mPageSize(pageSize), 
    mFileSize(fileSize), mBackendSize(fileSize), 
    mDebug("PageManager",this) 
{ 
    mDebug << __func__ << "(file: " << file.GetName() << ", size:" << fileSize << ", pageSize:" << pageSize << ")"; mDebug.Info();
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
void PageManager::ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock)
{
    if (mDebug) { mDebug << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    assert((index*mPageSize+offset+length <= mFileSize && length > 0));

    const Page& page { GetPageRead(index, dataLock) };
    const char* pageBuf { page.mData.data() };

    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    std::copy(pageBuf+iOffset, pageBuf+iOffset+iLength, buffer); 
}

/*****************************************************/
void PageManager::WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock)
{
    if (mDebug) { mDebug << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    assert((length > 0));

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const uint64_t newFileSize = std::max(mFileSize, pageStart+offset+length);

    const size_t pageSize { min64st(newFileSize-pageStart, mPageSize) };
    const bool partial { offset > 0 || length < pageSize };

    mDebug << __func__ << "... pageSize:" << pageSize << " newFileSize:" << newFileSize; mDebug.Info();

    Page& page { GetPageWrite(index, partial, dataLock) };
    page.mDirty = true;

    ResizePage(page, pageSize, dataLock);
    mFileSize = newFileSize; // extend file

    char* pageBuf { page.mData.data() }; // AFTER resize
    
    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    std::copy(buffer, buffer+iLength, pageBuf+iOffset);
}

/*****************************************************/
const Page& PageManager::GetPageRead(const uint64_t index, const SharedLockR& dataLock)
{
    mDebug << __func__ << "() " << mFile.GetID() << " (index:" << index << ")"; mDebug.Info();

    assert((index*mPageSize < mFileSize));

    UniqueLock pagesLock(mPagesMutex);

    // TODO optimize - maybe even if this index exists, always make sure at least the NEXT one is ready?
    // could generalize that with like a minPopulated or something, configurable?

    { PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        mDebug << __func__ << "... return existing page"; mDebug.Info();
        return it->second;
    } }

    if (!isPending(index, pagesLock))
    {
        const size_t fetchSize { GetFetchSize(index, pagesLock) };

        if (!fetchSize) // bigger than backend, create empty
        {
            mDebug << __func__ << "... create empty page"; mDebug.Info();
            return mPages.emplace(index, Page(mPageSize)).first->second;
        }

        StartFetch(index, fetchSize, pagesLock);
    }

    mDebug << __func__ << "... waiting for pending " << index; mDebug.Info();

    PageMap::iterator it;
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    mDebug << __func__ << "... returning pended page " << index; mDebug.Info();
    return it->second;
}

/*****************************************************/
Page& PageManager::GetPageWrite(const uint64_t index, const bool partial, const SharedLockW& dataLock)
{
    mDebug << __func__ << "() " << mFile.GetID() << " (index:" << index << " partial:" << partial << ")"; mDebug.Info();

    UniqueLock pagesLock(mPagesMutex);

    { PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        mDebug << __func__ << "... returning existing page"; mDebug.Info();
        return it->second;
    } }

    if (!isPending(index, pagesLock))
    {
        const uint64_t pageStart { index*mPageSize }; // offset of the page start
        if (pageStart >= mBackendSize || !partial) // bigger than backend, create empty
        {
            if (pageStart > mFileSize && mFileSize > 0)
            {
                PageMap::iterator it { mPages.find((mFileSize-1)/mPageSize) };
                if (it != mPages.end()) ResizePage(it->second, mPageSize, dataLock);
            }

            mDebug << __func__ << "... create empty page"; mDebug.Info();
            return mPages.emplace(index, Page(0)).first->second; // will be resized
        }

        mDebug << __func__ << "... partial write, reading single"; mDebug.Info();
        StartFetch(index, 1, pagesLock); // background thread
    }

    mDebug << __func__ << "... waiting for pending"; mDebug.Info();

    PageMap::iterator it;
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    mDebug << __func__ << "... returning pended page " << index; mDebug.Info();
    return it->second;
}

/*****************************************************/
void PageManager::ResizePage(Page& page, const uint64_t pageSize, const SharedLockW& dataLock)
{
    mDebug << __func__ << "(pageSize:" << pageSize << ")"; mDebug.Info();

    const uint64_t oldSize { page.mData.size() };
    page.mData.resize(pageSize);

    if (pageSize > oldSize) std::memset(
        page.mData.data()+oldSize, 0, pageSize-oldSize);
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
bool PageManager::isPending(const uint64_t index, const UniqueLock& pagesLock)
{
    for (const decltype(mPendingPages)::value_type& pend : mPendingPages)
        if (index >= pend.first && index < pend.first + pend.second) return true;
    return false;
}

/*****************************************************/
void PageManager::RemovePending(const uint64_t index, const UniqueLock& pagesLock)
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
size_t PageManager::GetFetchSize(const uint64_t index, const UniqueLock& pagesLock)
{
    assert((index*mPageSize < mFileSize));

    if (!mBackendSize) return 0;
    const uint64_t lastPage { (mBackendSize-1)/mPageSize }; // the last valid page index
    if (index > lastPage) return 0; // don't read-ahead empty pages

    const size_t maxReadCount { min64st(lastPage-index+1, mFetchSize) };

    mDebug << __func__ << "(index:" << index << ") backendSize:" << mBackendSize
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
void PageManager::StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock)
{
    mDebug << __func__ << "(index:" << index << ", readCount:" << readCount << ")"; mDebug.Info();

    assert((index*mPageSize < mFileSize && readCount > 0));

    mPendingPages.emplace_back(index, readCount);

    SharedLockR classLock(mClassMutex); // grab before creating
    std::thread(&PageManager::FetchPages, this, index, readCount, 
        std::move(classLock)).detach();
}

/*****************************************************/
void PageManager::FetchPages(const uint64_t index, const size_t count, SharedLockR classLock)
{
    SemaphorLock backendSem(sBackendSem);

    SharedLockR dataLock; // empty
    // if count is 1, waiter's dataLock has us covered
    if (count > 1) dataLock = SharedLockR(mDataMutex);

    assert((count > 0 && (index+count-1)*mPageSize < mBackendSize));
    
    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const size_t readSize { min64st(mBackendSize-pageStart, mPageSize*count) }; // length of data to fetch

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

            if (rindex == curIndex-index) // relevant read
            {
                char* pageBuf { curPage->mData.data() };
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

    mDebug << __func__ << "... thread returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::FlushPages(bool nothrow)
{
    mDebug << __func__ << "()"; mDebug.Info();

    // create a list of pages to write separate from mPages
    // so we don't have to hold pagesLock while sending
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
        auto writeFunc { [&]()->void { FlushPageList(writelist.back(), dataLock); } };

        if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const BaseException& e) { 
            mDebug << __func__ << "... Ignoring Error: " << e.what(); mDebug.Error(); }

        writelist.pop_back();
    }

    mDebug << __func__ << "... returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::FlushPageList(PageManager::PageRefMap& pages, const SharedLockR& dataLock)
{
    SemaphorLock backendSem(sBackendSem);

    mDebug << __func__ << "(pages:" << pages.size() << ")"; mDebug.Info();

    uint64_t totalSize { 0 };
    for (const PageRefMap::value_type& it : pages)
        totalSize += it.second.mData.size();

    uint64_t curOffset { 0 }; 
    std::string buf; buf.resize(totalSize);
    for (const PageRefMap::value_type& it : pages)
    {
        const decltype(Page::mData)& pageData { it.second.mData };
        std::copy(pageData.cbegin(), pageData.cend(), buf.begin() +
            static_cast<decltype(Page::mData)::iterator::difference_type>(curOffset));
        curOffset += pageData.size();
    }

    uint64_t writeStart { pages.begin()->first*mPageSize };
    mDebug << __func__ << "... WRITING " << buf.size() << " to " << writeStart; mDebug.Info();

    mBackend.WriteFile(mFile.GetID(), writeStart, buf);

    for (PageRefMap::value_type& it : pages)
        it.second.mDirty = false;

    mBackendSize = std::max(mBackendSize, writeStart+totalSize);
}

/*****************************************************/
bool PageManager::EvictPage(const uint64_t index)
{
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

    SharedLockW dataLock(mDataMutex); 
    // TODO if CacheManager is synchronous then can't grab this, File will have it
    // it actually seems a bit problematic that this even requires the writeLock...
    // but there may not be much choice, need to make sure nobody is accessing the page...
    // maybe CacheManager will have no choice but to be in a background thread...
    UniqueLock pagesLock(mPagesMutex);
    
    mDebug << __func__ << "... have locks"; mDebug.Info();

    PageMap::iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        Page& page { it->second };
        if (page.mDirty)
        {
            mDebug << __func__ << "... page is dirty, writing"; mDebug.Info();

            const uint64_t writeStart { it->first*mPageSize };
            const std::string data(reinterpret_cast<const char*>(page.mData.data()), page.mData.size());

            mBackend.WriteFile(mFile.GetID(), writeStart, data);
            // TODO return bool based on success/fail (don't throw)

            mBackendSize = std::max(mBackendSize, writeStart+data.size());
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

    mDebug << __func__ << "(newSize:" << backendSize << ") oldSize:" << mBackendSize << ")"; mDebug.Info();

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
            mDebug << __func__<< "... resize page:" << it->first << " size:" << pageSize; mDebug.Info();

            ResizePage(it->second, pageSize, dataLock);

            mDebug << __func__ << "... page resized!"; mDebug.Info();
        }
        else ++it;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
