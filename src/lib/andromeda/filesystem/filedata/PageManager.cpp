
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
    MDBG_INFO("(file: " << file.GetName() << ", size:" << fileSize << ", pageSize:" << pageSize << ")");
}

/*****************************************************/
PageManager::~PageManager()
{
    MDBG_INFO("() waiting for threads");

    // TODO interrupt existing threads? could set stream failbit to stop HTTPRunner
    SharedLockW dataLock(mDataMutex);

    if (mCacheMgr != nullptr)
    {
        for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); it++)
            mCacheMgr->RemovePage(it->second);
    }

    MDBG_INFO("... returning!");
}

/*****************************************************/
void PageManager::ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockR& dataLock)
{
    MDBG_INFO("(" << mFile.GetName() << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")");

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
    MDBG_INFO("(" << mFile.GetName() << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")");

    if (!length) throw std::invalid_argument(__func__);

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const uint64_t newFileSize = std::max(mFileSize, pageStart+offset+length);

    const size_t pageSize { min64st(newFileSize-pageStart, mPageSize) };
    const bool partial { offset > 0 || length < pageSize };

    MDBG_INFO("... pageSize:" << pageSize << " newFileSize:" << newFileSize);

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
    MDBG_INFO("(" << mFile.GetName() << ")" << " (index:" << index << ")");

    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__);

    UniqueLock pagesLock(mPagesMutex);

    // TODO optimize - maybe even if this index exists, always make sure at least the NEXT one is ready?
    // could generalize that with like a minPopulated or something, configurable?
    // FetchPages will need to take actual "haveLock" param not assume not to lock if size 1

    { PageMap::const_iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        MDBG_INFO("... return existing page");
        return it->second;
    } }

    if (!isPending(index, pagesLock))
    {
        const size_t fetchSize { GetFetchSize(index, pagesLock) };

        if (!fetchSize) // bigger than backend, create empty
        {
            MDBG_INFO("... create empty page");
            return mPages.emplace(index, mPageSize).first->second;
        }

        StartFetch(index, fetchSize, pagesLock);
    }

    MDBG_INFO("... waiting for pending " << index);

    PageMap::const_iterator it;
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    MDBG_INFO("... returning pended page " << index);
    return it->second;
}

/*****************************************************/
Page& PageManager::GetPageWrite(const uint64_t index, const bool partial, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName() << ")" << " (index:" << index << " partial:" << partial << ")");

    UniqueLock pagesLock(mPagesMutex);

    { PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        MDBG_INFO("... returning existing page");
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

            MDBG_INFO("... create empty page");
            return mPages.emplace(index, 0).first->second; // will be resized
        }

        MDBG_INFO("... partial write, reading single");
        StartFetch(index, 1, pagesLock); // background thread
    }

    MDBG_INFO("... waiting for pending");

    PageMap::iterator it;
    while ((it = mPages.find(index)) == mPages.end())
        mPagesCV.wait(pagesLock);

    MDBG_INFO("... returning pended page " << index);
    return it->second;
}

/*****************************************************/
void PageManager::ResizePage(Page& page, const size_t pageSize, bool inform)
{
    const size_t oldSize { page.size() };
    if (oldSize == pageSize) return; // no-op

    MDBG_INFO("(pageSize:" << pageSize << ") oldSize:" << oldSize);

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
    MDBG_INFO("(index:" << index << ")");

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

    MDBG_INFO("(index:" << index << ") backendSize:" << mBackendSize
        << " lastPage:" << lastPage << " maxReadCount:" << maxReadCount);

    size_t readCount { maxReadCount };
    for (const PageMap::value_type& page : mPages) // stop before next existing
    {
        if (page.first >= index)
        {
            MDBG_INFO("... first page at:" << page.first);
            readCount = min64st(page.first-index, readCount);
            break; // pages are in order
        }
    }

    for (size_t curCount { 0 }; curCount < readCount; ++curCount) // stop before next pending
    {
        if (isPending(curCount+index, pagesLock))
        {
            MDBG_INFO("... pending page at:" << curCount+index);
            readCount = curCount;
            break; // pages are in order
        }
    }

    MDBG_INFO("... return:"  << readCount);
    return readCount;
}

/*****************************************************/
void PageManager::StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock)
{
    MDBG_INFO("(index:" << index << ", readCount:" << readCount << ")");

    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__);

    mPendingPages.emplace_back(index, readCount);

    std::thread(&PageManager::FetchPages, this, index, readCount).detach();
}

/*****************************************************/
void PageManager::FetchPages(const uint64_t index, const size_t count)
{
    SemaphorLock backendSem(sBackendSem);

    MDBG_INFO("()");

    std::unique_ptr<SharedLockRP> dataLock;
    // if count is 1, waiter's dataLock has us covered
    // use a read-priority lcok since the caller is waiting on us,
    // if another write happens in the middle we would deadlock
    if (count > 1) dataLock = std::make_unique<SharedLockRP>(mDataMutex);

    if (!count || (index+count-1)*mPageSize >= mBackendSize) 
        throw std::invalid_argument(__func__);

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const size_t readSize { min64st(mBackendSize-pageStart, mPageSize*count) }; // length of data to fetch

    MDBG_INFO("... threads:" << sBackendSem.get_count() << " index:" << index 
        << " count:" << count << " pageStart:" << pageStart << " readSize:" << readSize);

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
            else { MDBG_INFO("... old read, ignoring"); }

            rbuf += pwLength; rbyte += pwLength;
        }
    });

    if (curPage != nullptr) { assert(false); MDBG_ERROR("() ERROR unfinished read!"); } // stop only in debug builds

    MDBG_INFO("... thread returning!");
}

/*****************************************************/
void PageManager::FlushPages(bool nothrow)
{
    MDBG_INFO("()");

    // create runs of pages to write separate from mPages
    // so we don't have to hold pagesLock while sending
    // ... map iterators stay valid after other operations
    std::map<uint64_t, PagePtrList> writeLists;

    SharedLockR dataLock(mDataMutex);

    { // lock scope
        UniqueLock pagesLock(mPagesMutex);

        MDBG_INFO("... have locks");

        size_t curSize { 0 };
        uint64_t lastIndex { 0 };
        PagePtrList* curList { nullptr };
        for (PageMap::value_type& pagePair : mPages)
        {
            if (pagePair.second.mDirty)
            {
                size_t pageSize { pagePair.second.size() };

                if (curList == nullptr
                    || lastIndex+1 != pagePair.first // not contiguous
                    || curSize + pageSize < curSize) // size_t overflow!
                {
                    MDBG_INFO("... new write run at " << pagePair.first);
                    curList = &(writeLists.emplace(pagePair.first, PagePtrList()).first->second);
                    curSize = 0;
                }

                lastIndex = pagePair.first;
                curList->push_back(&pagePair.second);
                curSize += pagePair.second.size();
            }
            else curList = nullptr; // end run
        }
    }

    for (const decltype(writeLists)::value_type& writePair : writeLists)
    {
        auto writeFunc { [&]()->void { FlushPageList(writePair.first, writePair.second, dataLock); } };

        if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const BaseException& e) { 
            MDBG_ERROR("... Ignoring Error: " << e.what()); }
    }

    MDBG_INFO("... returning!");
}

/*****************************************************/
void PageManager::FlushPageList(const uint64_t index, const PageManager::PagePtrList& pages, const SharedLockR& dataLock)
{
    SemaphorLock backendSem(sBackendSem);

    MDBG_INFO("(index:" << index << " pages:" << pages.size() << ")");

    size_t totalSize { 0 };
    for (const Page* pagePtr : pages)
        totalSize += pagePtr->size();

    std::string buf; buf.resize(totalSize); char* curBuf { buf.data() };
    for (const Page* pagePtr : pages)
    {
        std::copy(pagePtr->mData.cbegin(), pagePtr->mData.cend(), curBuf);
        curBuf += pagePtr->size();
    }

    uint64_t writeStart { index*mPageSize };
    MDBG_INFO("... WRITING " << buf.size() << " to " << writeStart);

    mBackend.WriteFile(mFile.GetID(), writeStart, buf);

    for (Page* pagePtr : pages)
        pagePtr->mDirty = false;

    mBackendSize = std::max(mBackendSize, writeStart+totalSize);
}

/*****************************************************/
bool PageManager::EvictPage(const uint64_t index, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName() << ") (index:" << index << ")");

    UniqueLock pagesLock(mPagesMutex);

    PageMap::const_iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end())
    {
        const Page& page { pageIt->second };
        
        if (page.mDirty)
        {
            MDBG_INFO("... page is dirty, writing");

            const uint64_t writeStart { pageIt->first*mPageSize };
            const std::string data(reinterpret_cast<const char*>(page.data()), page.size());

            mBackend.WriteFile(mFile.GetID(), writeStart, data);
            // TODO return bool based on success/fail (don't throw)

            mBackendSize = std::max(mBackendSize, writeStart+data.size());
        }

        mPages.erase(pageIt);

        MDBG_INFO("... page removed, numPages:" << mPages.size());
    }
    else { MDBG_INFO(" ... page not found"); }

    return true;
}

/*****************************************************/
void PageManager::RemoteChanged(const uint64_t backendSize)
{    
    // TODO add ability to interrupt in-progress reads (same as destructor)
    // since the download is streamed you should be able to just set a flag to have it reply false to httplib
    // then catch the httplib::Canceled and ignore it and return if that flag is set - will help error handling later

    MDBG_INFO("(newSize:" << backendSize << ") oldSize:" << mBackendSize << ")");

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);

    MDBG_INFO("... have locks");

    uint64_t maxDirty { 0 }; // evict all pages
    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        Page& page { it->second };
        if (!page.mDirty)
        {
            if (mCacheMgr) mCacheMgr->RemovePage(page);
            it = mPages.erase(it);
        }
        else
        {
            MDBG_ERROR("... WARNING remote changed while we have dirty pages!");

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
    MDBG_INFO("(newSize:" << newSize << ")");

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);

    MDBG_INFO("... have locks");

    mBackend.TruncateFile(mFile.GetID(), newSize); 

    mBackendSize = newSize;
    mFileSize = newSize;

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/mPageSize) // remove past end
        {
            MDBG_INFO("... erase page:" << it->first);

            if (mCacheMgr) mCacheMgr->RemovePage(it->second);
            it = mPages.erase(it);
        }
        else if (it->first == (newSize-1)/mPageSize) // resize last page
        {
            size_t pageSize { static_cast<size_t>(newSize - it->first*mPageSize) };
            MDBG_INFO("... resize page:" << it->first << " size:" << pageSize);

            ResizePage(it->second, pageSize); ++it;
        }
        else ++it;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
