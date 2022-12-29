
#include <cstring>
#include <nlohmann/json.hpp>

#include "CacheManager.hpp"
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
PageManager::PageManager(File& file, const uint64_t fileSize, const size_t pageSize) : 
    mFile(file), mBackend(file.GetBackend()), mCacheMgr(mBackend.GetCacheManager()),
    mPageSize(pageSize), mFileSize(fileSize), mBackendSize(fileSize), 
    mDebug("PageManager",this) 
{ 
    mDebug << __func__ << "(file: " << file.GetName() << ", size:" << fileSize << ", pageSize:" << pageSize << ")"; mDebug.Info();
}

/*****************************************************/
PageManager::~PageManager()
{
    mDebug << __func__ << "() waiting for threads"; mDebug.Info();

    // TODO interrupt existing threads? could set stream failbit to stop HTTPRunner
    SharedLockW dataLock(mDataMutex);

    if (mCacheMgr != nullptr)
    {
        for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); it++)
            mCacheMgr->ErasePage(it->second);
    }

    mDebug << __func__ << "... returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock)
{
    if (mDebug) { mDebug << __func__ << "(" << mFile.GetName() << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    if (index*mPageSize+offset+length > mFileSize || !length) 
        throw std::invalid_argument(__func__);

    const Page& page { GetPageRead(index, dataLock) };
    const char* pageBuf { page.data() };

    if (mCacheMgr) mCacheMgr->InformPage(*this, index, page);

    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    std::copy(pageBuf+iOffset, pageBuf+iOffset+iLength, buffer); 
}

/*****************************************************/
void PageManager::WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock)
{
    if (mDebug) { mDebug << __func__ << "(" << mFile.GetName() << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    if (!length) throw std::invalid_argument(__func__);

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const uint64_t newFileSize = std::max(mFileSize, pageStart+offset+length);

    const size_t pageSize { min64st(newFileSize-pageStart, mPageSize) };
    const bool partial { offset > 0 || length < pageSize };

    mDebug << __func__ << "... pageSize:" << pageSize << " newFileSize:" << newFileSize; mDebug.Info();

    Page& page { GetPageWrite(index, partial, dataLock) };

    // false to not inform cacheMgr here - will below
    ResizePage(page, pageSize, false);
    mFileSize = newFileSize; // extend file

    char* pageBuf { page.data() };
    page.mDirty = true;

    if (mCacheMgr) mCacheMgr->InformPage(*this, index, page);
    
    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    std::copy(buffer, buffer+iLength, pageBuf+iOffset);
}

/*****************************************************/
const Page& PageManager::GetPageRead(const uint64_t index, const SharedLockR& dataLock)
{
    mDebug << __func__ << "(" << mFile.GetName() << ")" << " (index:" << index << ")"; mDebug.Info();

    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__);

    UniqueLock pagesLock(mPagesMutex);

    // TODO optimize - maybe even if this index exists, always make sure at least the NEXT one is ready?
    // could generalize that with like a minPopulated or something, configurable?
    // FetchPages will need to take actual "haveLock" param not assume not to lock if size 1

    { PageMap::const_iterator it { mPages.find(index) };
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
            return mPages.emplace(index, mPageSize).first->second;
        }

        StartFetch(index, fetchSize, pagesLock);
    }

    mDebug << __func__ << "... waiting for pending " << index; mDebug.Info();

    PageMap::const_iterator it;
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    mDebug << __func__ << "... returning pended page " << index; mDebug.Info();
    return it->second;
}

/*****************************************************/
Page& PageManager::GetPageWrite(const uint64_t index, const bool partial, const SharedLockW& dataLock)
{
    mDebug << __func__ << "(" << mFile.GetName() << ")" << " (index:" << index << " partial:" << partial << ")"; mDebug.Info();

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
                if (it != mPages.end()) ResizePage(it->second, mPageSize);
            }

            mDebug << __func__ << "... create empty page"; mDebug.Info();
            return mPages.emplace(index, 0).first->second; // will be resized
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
void PageManager::ResizePage(Page& page, const uint64_t pageSize, bool inform)
{
    const uint64_t oldSize { page.size() };
    if (oldSize == pageSize) return; // no-op

    mDebug << __func__ << "(pageSize:" << pageSize << ") oldSize:" << oldSize; mDebug.Info();

    page.mData.resize(pageSize);

    if (pageSize > oldSize) std::memset(
        page.data()+oldSize, 0, pageSize-oldSize);

    if (inform && mCacheMgr) mCacheMgr->ResizePage(page, pageSize);
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
    mDebug << __func__ << "(index:" << index << ")"; mDebug.Info();

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
    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__);

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

    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__);

    mPendingPages.emplace_back(index, readCount);

    std::thread(&PageManager::FetchPages, this, index, readCount).detach();
}

/*****************************************************/
void PageManager::FetchPages(const uint64_t index, const size_t count)
{
    SemaphorLock backendSem(sBackendSem);

    mDebug << __func__ << "()"; mDebug.Info();

    SharedLockR dataLock; // empty
    // if count is 1, waiter's dataLock has us covered
    if (count > 1) dataLock = SharedLockR(mDataMutex);

    if (!count || (index+count-1)*mPageSize >= mBackendSize) 
        throw std::invalid_argument(__func__);

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
            const uint64_t curPageStart { curIndex*mPageSize };
            const size_t pageSize { min64st(mBackendSize-curPageStart, mPageSize) };

            if (!curPage) curPage = std::make_unique<Page>(pageSize);

            const uint64_t rindex { rbyte / mPageSize }; // page index for this data
            const size_t pwOffset { static_cast<size_t>(rbyte - rindex*mPageSize) }; // offset within the page
            const size_t pwLength { min64st(rlength+roffset-rbyte, mPageSize-pwOffset) }; // length within the page

            if (rindex == curIndex-index) // relevant read
            {
                char* pageBuf { curPage->data() };
                memcpy(pageBuf+pwOffset, rbuf, pwLength);

                if (pwOffset+pwLength == curPage->size()) // page is done
                {
                    UniqueLock pagesLock(mPagesMutex);

                    // if we are reading a page that is smaller on the backend (dirty writes), might need to resize
                    const size_t realSize { min64st(mFileSize-curPageStart, mPageSize) };
                    if (pageSize < realSize) ResizePage(*curPage, realSize, false);

                    PageMap::const_iterator newIt { mPages.emplace(curIndex, std::move(*curPage)).first };
                    if (mCacheMgr) mCacheMgr->InformPage(*this, curIndex, newIt->second);

                    RemovePending(curIndex, pagesLock); 
                    curPage.reset(); ++curIndex;
                }
            }
            else { mDebug << __func__ << "... old read, ignoring"; mDebug.Info(); }

            rbuf += pwLength; rbyte += pwLength;
        }
    });

    if (curPage != nullptr) { assert(false); mDebug << __func__ << "() unfinished read!"; mDebug.Error(); } // stop only in debug builds

    mDebug << __func__ << "... thread returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::FlushPages(bool nothrow)
{
    mDebug << __func__ << "()"; mDebug.Info();

    // create runs of pages to write separate from mPages
    // so we don't have to hold pagesLock while sending
    // ... map iterators stay valid after other operations
    std::map<uint64_t, PagePtrList> writeLists;

    SharedLockR dataLock(mDataMutex);

    { // lock scope
        UniqueLock pagesLock(mPagesMutex);

        mDebug << __func__ << "... have locks"; mDebug.Info();

        uint64_t lastIndex { 0 };
        PagePtrList* curList { nullptr };
        for (PageMap::value_type& pagePair : mPages)
        {
            if (pagePair.second.mDirty)
            {
                if (!curList || lastIndex+1 != pagePair.first)
                {
                    mDebug << __func__ << "... new write run at " << pagePair.first; mDebug.Info();
                    curList = &(writeLists.emplace(pagePair.first, PagePtrList()).first->second);
                }

                lastIndex = pagePair.first;
                curList->push_back(&pagePair.second);
            }
            else curList = nullptr; // end run
        }
    }

    for (const decltype(writeLists)::value_type& writePair : writeLists)
    {
        auto writeFunc { [&]()->void { FlushPageList(writePair.first, writePair.second, dataLock); } };

        if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const BaseException& e) { 
            mDebug << __func__ << "... Ignoring Error: " << e.what(); mDebug.Error(); }
    }

    mDebug << __func__ << "... returning!"; mDebug.Info();
}

/*****************************************************/
void PageManager::FlushPageList(const uint64_t index, const PageManager::PagePtrList& pages, const SharedLockR& dataLock)
{
    SemaphorLock backendSem(sBackendSem);

    mDebug << __func__ << "(index:" << index << " pages:" << pages.size() << ")"; mDebug.Info();

    uint64_t totalSize { 0 };
    for (const Page* pagePtr : pages)
        totalSize += pagePtr->size();

    std::string buf; buf.resize(totalSize); char* curBuf { buf.data() };
    for (const Page* pagePtr : pages)
    {
        std::copy(pagePtr->mData.cbegin(), pagePtr->mData.cend(), curBuf);
        curBuf += pagePtr->size();
    }

    uint64_t writeStart { index*mPageSize };
    mDebug << __func__ << "... WRITING " << buf.size() << " to " << writeStart; mDebug.Info();

    mBackend.WriteFile(mFile.GetID(), writeStart, buf);

    for (Page* pagePtr : pages)
        pagePtr->mDirty = false;

    mBackendSize = std::max(mBackendSize, writeStart+totalSize);
}

/*****************************************************/
bool PageManager::EvictPage(const uint64_t index, const SharedLockW& dataLock)
{
    mDebug << __func__ << "(" << mFile.GetName() << ") (index:" << index << ")"; mDebug.Info();

    UniqueLock pagesLock(mPagesMutex);

    PageMap::const_iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end())
    {
        const Page& page { pageIt->second };
        
        if (page.mDirty)
        {
            mDebug << __func__ << "... page is dirty, writing"; mDebug.Info();

            const uint64_t writeStart { pageIt->first*mPageSize };
            const std::string data(reinterpret_cast<const char*>(page.data()), page.size());

            mBackend.WriteFile(mFile.GetID(), writeStart, data);
            // TODO return bool based on success/fail (don't throw)

            mBackendSize = std::max(mBackendSize, writeStart+data.size());
        }

        mPages.erase(pageIt);

        mDebug << __func__ << "... page removed, numPages:" << mPages.size(); mDebug.Info();
    }
    else { mDebug << __func__ << " ... page not found"; mDebug.Info(); }

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
            if (mCacheMgr) mCacheMgr->ErasePage(page);
            it = mPages.erase(it);
        }
        else
        {
            mDebug << __func__ << "... WARNING remote changed while we have dirty pages!"; mDebug.Error();

            uint64_t pageMax { it->first*mPageSize + page.size() };
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

            if (mCacheMgr) mCacheMgr->ErasePage(it->second);
            it = mPages.erase(it); 

            mDebug << __func__ << "... page removed!" << it->first; mDebug.Info();
        }
        else if (it->first == (newSize-1)/mPageSize) // resize last page
        {
            uint64_t pageSize { newSize - it->first*mPageSize };
            mDebug << __func__<< "... resize page:" << it->first << " size:" << pageSize; mDebug.Info();

            ResizePage(it->second, pageSize);

            mDebug << __func__ << "... page resized!"; mDebug.Info();
        }
        else ++it;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
