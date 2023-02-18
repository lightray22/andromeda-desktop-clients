
#include <cassert>
#include <cstring>
#include <limits>

#include "CacheManager.hpp"
#include "PageManager.hpp"
#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
PageManager::PageManager(File& file, const uint64_t fileSize, const size_t pageSize, bool backendExists) :
    mDebug("PageManager",this),
    mFile(file),
    mBackend(file.GetBackend()),
    mCacheMgr(mBackend.GetCacheManager()),
    mPageSize(pageSize), 
    mFileSize(fileSize), 
    mBandwidth("PageManager"),
    mPageBackend(file, pageSize, fileSize, backendExists)
{ 
    MDBG_INFO("(file:" << file.GetName() << ", size:" << fileSize << ", pageSize:" << pageSize << ")");
}

/*****************************************************/
PageManager::~PageManager()
{
    MDBG_INFO("() waiting for lock");

    mScopeMutex.lock(); // exclusive, forever
    SharedLockW dataLock(mDataMutex);

    if (mCacheMgr != nullptr)
    {
        for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); ++it)
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

    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    const char* pageBuf { page.data() };
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

    Page& page { GetPageWrite(index, pageSize, partial, dataLock) };
    mFileSize = newFileSize; // extend file

    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    char* pageBuf { page.data() };
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

        if (mCacheMgr && !mBackend.isMemory()) 
            mCacheMgr->InformPage(*this, index, it->second);
        return it->second;
    } }

    if (!isFetchPending(index, pagesLock))
    {
        const size_t fetchSize { GetFetchSize(index, pagesLock) };
        if (!fetchSize) // bigger than backend, create empty
        {
            MDBG_INFO("... create empty page");
            Page& newPage { mPages.emplace(index, 0).first->second };

            ResizePage(newPage, mPageSize, false); // zeroize
            // InformPage() is never synchronous without a dataLockW so holding pagesLock is okay
            InformNewPage(index, newPage, &pagesLock, nullptr);
            return newPage;
        }
        else StartFetch(index, fetchSize, pagesLock);
    }

    MDBG_INFO("... waiting for pending " << index);

    PageMap::const_iterator it;
    std::exception_ptr fail;

    while ((it = mPages.find(index)) == mPages.end() &&
            !(fail = isFetchFailed(index, pagesLock)))
        mPagesCV.wait(pagesLock);

    if (fail != nullptr)
    {
        MDBG_INFO("... rethrowing exception");
        std::rethrow_exception(fail);
    }

    MDBG_INFO("... returning pended page " << index);

    if (mCacheMgr && !mBackend.isMemory()) 
        mCacheMgr->InformPage(*this, index, it->second);
    return it->second;
}

/*****************************************************/
Page& PageManager::GetPageWrite(const uint64_t index, const size_t pageSize, const bool partial, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName() << ")" << " (index:" << index << " pageSize:" << pageSize << " partial:" << partial << ")");

    // don't need pagesLock, have an exclusive dataLock... also holding pagesLock
    // could lead to deadlock since InformNewPage() may synchronously evict/flush

    { PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        MDBG_INFO("... returning existing page");
        it->second.mDirty = true;

        InformResizePage(index, it->second, pageSize, dataLock);
        return it->second;
    } }

    // as we have an exclusive dataLock, we know there are no background reads,
    // so the page is not already pending, and we can read synchronously

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    if (pageStart >= mPageBackend.GetBackendSize() || !partial)
    {
        if (pageStart > mFileSize && mFileSize > 0)
        {
            // extend the old last page if necessary
            PageMap::iterator it { mPages.find((mFileSize-1)/mPageSize) };
            if (it != mPages.end()) ResizePage(it->second, mPageSize, true, &dataLock);
        }

        MDBG_INFO("... create empty page");
        Page& newPage { mPages.emplace(index, 0).first->second };
        newPage.mDirty = true;
        
        ResizePage(newPage, pageSize, false); // zeroize
        InformNewPage(index, newPage, nullptr, &dataLock);
        return newPage;
    }

    MDBG_INFO("... partial write, reading single");
    Page* newPage; mPageBackend.FetchPages(index, 1, // read a single page
        [&](const uint64_t pageIndex, const uint64_t pageStartI, const size_t pageSizeI, Page& page)
    {
        if (pageSizeI < pageSize) ResizePage(page, pageSize, false);
        newPage = &(mPages.emplace(pageIndex, std::move(page)).first->second);
        newPage->mDirty = true;
    });

    ResizePage(*newPage, pageSize, false);
    InformNewPage(index, *newPage, nullptr, &dataLock);
    MDBG_INFO("... returning pended page " << index);
    return *newPage;
}

/*****************************************************/
void PageManager::InformNewPage(const uint64_t index, const Page& page, const UniqueLock* pagesLock, const SharedLockW* dataLock)
{
    assert(pagesLock != nullptr || dataLock != nullptr);

    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, true, dataLock); }
    catch (BaseException& ex)
    {
        mCacheMgr->RemovePage(page);
        mPages.erase(index); // undo memory usage
        std::rethrow_exception(std::current_exception());
    }
}

/*****************************************************/
void PageManager::InformResizePage(const uint64_t index, Page& page, const size_t pageSize, const SharedLockW& dataLock)
{
    const size_t oldSize { page.size() };
    ResizePage(page, pageSize, false);

    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, true, &dataLock); }
    catch (BaseException& ex)
    {
        mCacheMgr->ResizePage(*this, page, oldSize, &dataLock);
        ResizePage(page, oldSize, false); // undo memory usage
        std::rethrow_exception(std::current_exception());
    }
}

/*****************************************************/
void PageManager::ResizePage(Page& page, const size_t pageSize, bool inform, const SharedLockW* dataLock)
{
    const size_t oldSize { page.size() };
    if (oldSize == pageSize) return; // no-op

    MDBG_INFO("(pageSize:" << pageSize << ") oldSize:" << oldSize);

    // inform the cache manager before resizing in case of an error
    if (inform && mCacheMgr) 
        mCacheMgr->ResizePage(*this, page, pageSize, dataLock);

    page.mData.resize(pageSize);

    if (pageSize > oldSize) std::memset(
        page.data()+oldSize, 0, pageSize-oldSize);
}

/*****************************************************/
bool PageManager::isDirty(const uint64_t index, const SharedLockW& dataLock)
{
    UniqueLock pagesLock(mPagesMutex);

    const PageMap::const_iterator it { mPages.find(index) };
    if (it == mPages.cend()) return false;

    return it->second.mDirty;
}

/*****************************************************/
bool PageManager::isFetchPending(const uint64_t index, const UniqueLock& pagesLock)
{
    for (const decltype(mPendingPages)::value_type& pend : mPendingPages)
        if (index >= pend.first && index < pend.first + pend.second) return true;
    return false;
}

/*****************************************************/
std::exception_ptr PageManager::isFetchFailed(const uint64_t index, const UniqueLock& pagesLock)
{
    FailureMap::iterator it { mFailedPages.find(index) };
    return (it != mFailedPages.end()) ? it->second : nullptr;
}

/*****************************************************/
void PageManager::RemovePendingFetch(const uint64_t index, bool idxOnly, const UniqueLock& pagesLock)
{
    MDBG_INFO("(index:" << index << ")");

    for (PendingMap::iterator pend { mPendingPages.begin() }; 
        pend != mPendingPages.end(); ++pend)
    {
        if (index == pend->first)
        {
            ++pend->first; // index
            --pend->second; // count

            if (!idxOnly || !pend->second)
                mPendingPages.erase(pend);

            mPagesCV.notify_all(); return;
        }
    }
}

/*****************************************************/
size_t PageManager::GetFetchSize(const uint64_t index, const UniqueLock& pagesLock)
{
    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__);

    const uint64_t backendSize { mPageBackend.GetBackendSize() };

    if (!backendSize) return 0;
    const uint64_t lastPage { (backendSize-1)/mPageSize }; // the last valid page index
    if (index > lastPage) return 0; // don't read-ahead empty pages

    const size_t maxReadCount { min64st(lastPage-index+1, mFetchSize) };

    MDBG_INFO("(index:" << index << ") backendSize:" << backendSize
        << " lastPage:" << lastPage << " maxReadCount:" << maxReadCount);

    // no read-ahead for the first page as file managers often read just metadata
    size_t readCount { (index > 0) ? maxReadCount : 1 };

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
        if (isFetchPending(curCount+index, pagesLock))
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

    if (mFailedPages.size())
    {
        size_t erased { 0 };
        for (uint64_t idx { index }; idx < index+readCount; ++idx)
            erased += mFailedPages.erase(idx);
        if (erased) MDBG_INFO("... reset " << erased << " failures");
    }

    std::thread(&PageManager::FetchPages, this, index, readCount).detach();
}

/*****************************************************/
void PageManager::FetchPages(const uint64_t index, const size_t count)
{
    MDBG_INFO("(index:" << index << " count:" << count << ")");

    // use a read-priority lock since the caller is waiting on us, 
    // if another write happens in the middle we would deadlock
    SharedLockRP dataLock(mDataMutex);

    std::chrono::steady_clock::time_point timeStart { std::chrono::steady_clock::now() };

    uint64_t nextIndex { index }; try
    {
        const size_t readSize { mPageBackend.FetchPages(index, count, 
            [&](const uint64_t pageIndex, const uint64_t pageStart, const size_t pageSize, Page& page)
        {
            // if we are reading a page that is smaller on the backend (dirty writes), might need to extend
            const size_t realSize { min64st(mFileSize-pageStart, mPageSize) };
            if (pageSize < realSize) ResizePage(page, realSize, false);

            PageMap::const_iterator newIt;
            { // lock scope 
                UniqueLock pagesLock(mPagesMutex);
                newIt = mPages.emplace(pageIndex, std::move(page)).first;
                RemovePendingFetch(pageIndex, true, pagesLock); 
            } // unlock to let waiters proceed with page

            // assume the requesting thread will do InformPage() for the first page
            if (mCacheMgr && !mBackend.isMemory() && pageIndex > index)
                mCacheMgr->InformPage(*this, pageIndex, newIt->second, false);
            // pass false to not block - we have the httplib lock and reclaiming
            // memory may involve httplib flushing pages - would deadlock

            ++nextIndex;
        }) };

        if (readSize >= mPageSize) // don't consider small reads
            UpdateBandwidth(readSize, std::chrono::steady_clock::now()-timeStart);
    }
    catch (const BaseException& ex)
    {
        MDBG_ERROR("... " << ex.what());
        UniqueLock pagesLock(mPagesMutex);

        std::exception_ptr exptr { std::current_exception() };
        for (uint64_t eidx { nextIndex }; eidx < nextIndex+count; ++eidx)
            mFailedPages[eidx] = exptr; // shared_ptr

        RemovePendingFetch(nextIndex, false, pagesLock);
    }
    
    MDBG_INFO("... thread returning!");
}

/*****************************************************/
void PageManager::UpdateBandwidth(const size_t bytes, const std::chrono::steady_clock::duration& time)
{
    UniqueLock llock(mFetchSizeMutex);

    uint64_t targetBytes { mBandwidth.UpdateBandwidth(bytes, time) };

    if (mCacheMgr)
    {
        const uint64_t cacheMax { mCacheMgr->GetMemoryLimit()/mReadMaxCacheFrac };
        if (targetBytes > cacheMax)
        {
            targetBytes = cacheMax;
            MDBG_INFO("... cache limited targetBytes:" << cacheMax);
        }
    }

    uint64_t fetchSize { std::max(static_cast<uint64_t>(1), targetBytes/mPageSize) };
    mFetchSize = min64st(fetchSize, std::numeric_limits<size_t>::max()/mPageSize); // size_t readSize
    MDBG_INFO("... newFetchSize:" << fetchSize << " (32-bit limit:" << mFetchSize <<")");
}

/*****************************************************/
uint64_t PageManager::GetWriteList(PageMap::iterator& pageIt, PageBackend::PagePtrList& writeList, const UniqueLock& pagesLock)
{
    MDBG_INFO("(pageItIndex:" << pageIt->first << ")");

    size_t curSize { 0 };
    uint64_t lastIndex { 0 };

    uint64_t startIndex { (mFileSize-1)/mPageSize+1 }; // invalid

    for (; pageIt != mPages.end(); ++pageIt)
    {
        if (pageIt->second.mDirty)
        {
            size_t pageSize { pageIt->second.size() };

            if (writeList.empty())
            {
                startIndex = pageIt->first;
                MDBG_INFO("... start write run at " << startIndex);
            }
            else if (lastIndex+1 != pageIt->first || // not consecutive
                     curSize + pageSize < curSize) break; // size_t overflow!
            else
            {
                const size_t maxWrite { mBackend.GetConfig().GetUploadMaxBytes() };
                // no point in doing a bigger write than the backend can send at once
                if (maxWrite && curSize + pageSize > maxWrite) break;
            }

            lastIndex = pageIt->first;
            writeList.push_back(&pageIt->second);
            curSize += pageIt->second.size();
        }
        else if (!writeList.empty()) break; // end run
    }

    MDBG_INFO("... return size: " << writeList.size());
    return startIndex;
}

/*****************************************************/
bool PageManager::EvictPage(const uint64_t index, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName() << ") (index:" << index << ")");

    UniqueLock flushLock(mFlushMutex);
    UniqueLock pagesLock(mPagesMutex); // can hold, have dataLockW anyway

    PageMap::iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end())
    {    
        if (pageIt->second.mDirty)
        {
            MDBG_INFO("... page is dirty, writing");

            PageBackend::PagePtrList writeList;
            PageMap::iterator pageItTmp { pageIt }; // copy
            GetWriteList(pageItTmp, writeList, pagesLock);
            FlushPageList(index, writeList, flushLock);
        }

        if (mCacheMgr) mCacheMgr->RemovePage(pageIt->second);
        mPages.erase(pageIt); // need dataLockW for this

        MDBG_INFO("... page removed, numPages:" << mPages.size());
    }
    else { MDBG_INFO(" ... page not found"); }

    return true;
}

/*****************************************************/
size_t PageManager::FlushPage(const uint64_t index, const SharedLockRP& dataLock)
{
    UniqueLock flushLock(mFlushMutex);
    return FlushPage(index, flushLock);
}

/*****************************************************/
size_t PageManager::FlushPage(const uint64_t index, const SharedLockW& dataLock)
{
    UniqueLock flushLock(mFlushMutex);
    return FlushPage(index, flushLock);
}

/*****************************************************/
size_t PageManager::FlushPage(const uint64_t index, const UniqueLock& flushLock)
{
    MDBG_INFO("(" << mFile.GetName() << ") (index:" << index << ")");

    PageBackend::PagePtrList writeList;

    { // lock scope
        UniqueLock pagesLock(mPagesMutex);

        PageMap::iterator pageIt { mPages.find(index) };
        if (pageIt != mPages.end() && pageIt->second.mDirty)
        {
            GetWriteList(pageIt, writeList, pagesLock);
        }
        else { MDBG_INFO(" ... page not found"); }
    }

    // truncate is only cached before mBackendExists
    const bool flushTruncate { !mPageBackend.ExistsOnBackend() };

    // flush pages first as this may handle truncating
    size_t written { writeList.empty() ? 0 :
        FlushPageList(index, writeList, flushLock) };

    if (flushTruncate) FlushTruncate(flushLock);

    MDBG_INFO("... return:" << written); return written;
}

/*****************************************************/
void PageManager::FlushPages()
{
    MDBG_INFO("()");

    SharedLockR dataLock(mDataMutex);
    UniqueLock flushLock(mFlushMutex);

    // create runs of pages to write separate from mPages so we don't have to hold pagesLock
    std::map<uint64_t, PageBackend::PagePtrList> writeLists;

    { // lock scope
        UniqueLock pagesLock(mPagesMutex);

        MDBG_INFO("... have locks");

        PageMap::iterator pageIt { mPages.begin() };
        while (pageIt != mPages.end())
        {
            PageBackend::PagePtrList writeList;
            const uint64_t startIndex {
                GetWriteList(pageIt, writeList, pagesLock) };

            if (!writeList.empty())
                writeLists.emplace(startIndex, std::move(writeList));
            // this loop is not N^2 as GetWriteList() increments our pageIt
        }

        MDBG_INFO("... write runs:" << writeLists.size());
    }

    // truncate is only cached before mBackendExists
    const bool flushTruncate { !mPageBackend.ExistsOnBackend() };

    // flush pages first as this may handle truncating
    for (const decltype(writeLists)::value_type& writePair : writeLists)
        FlushPageList(writePair.first, writePair.second, flushLock);

    if (flushTruncate) FlushTruncate(flushLock);
    
    MDBG_INFO("... returning!");
}

/*****************************************************/
size_t PageManager::FlushPageList(const uint64_t index, const PageBackend::PagePtrList& pages, const PageManager::UniqueLock& flushLock)
{
    MDBG_INFO("(index:" << index << " pages:" << pages.size() << ")");

    const size_t totalSize { mPageBackend.FlushPageList(index, pages) };

    for (Page* pagePtr : pages)
    {
        pagePtr->mDirty = false;
        if (mCacheMgr) mCacheMgr->RemoveDirty(*pagePtr);
    }

    return totalSize;
}

/*****************************************************/
void PageManager::FlushTruncate(const PageManager::UniqueLock& flushLock)
{
    MDBG_INFO("()");

    mPageBackend.FlushCreate();

    uint64_t maxDirty { 0 }; // byte after last dirty byte
    { // lock scope
        UniqueLock pagesLock(mPagesMutex);
        for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); ++it)
        {
            Page& page { it->second };
            if (page.mDirty)
            {
                uint64_t pageMax { it->first*mPageSize + page.size() };
                maxDirty = std::max(maxDirty, pageMax);
            }
        }
    }

    // mFileSize should only be larger than mBackendSize if dirty writes
    if (std::max(mPageBackend.GetBackendSize(), maxDirty) < mFileSize)
    {
        MDBG_INFO("... truncate maxDirty:" << maxDirty << " fileSize:" << mFileSize);
        mPageBackend.Truncate(mFileSize);
    }
}

/*****************************************************/
void PageManager::RemoteChanged(const uint64_t backendSize)
{
    if (!mPageBackend.ExistsOnBackend()) return; // called Refresh() ourselves

    MDBG_INFO("(newSize:" << backendSize << ") oldSize:" << mPageBackend.GetBackendSize() << ")");

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);

    MDBG_INFO("... have locks");

    uint64_t maxDirty { 0 };  // byte after last dirty byte
    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        Page& page { it->second };
        if (!page.mDirty) // evict all non-dirty
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

    mFileSize = std::max(backendSize, maxDirty);
    mPageBackend.SetBackendSize(backendSize);
}

/*****************************************************/
void PageManager::Truncate(const uint64_t newSize)
{
    MDBG_INFO("(newSize:" << newSize << ")");

    SharedLockW dataLock(mDataMutex);
    UniqueLock pagesLock(mPagesMutex);

    MDBG_INFO("... have locks");

    mPageBackend.Truncate(newSize);
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

            ResizePage(it->second, pageSize, true, &dataLock); ++it;
        }
        else ++it;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
