
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
PageManager::PageManager(File& file, const uint64_t fileSize, const size_t pageSize, PageBackend& pageBackend) :
    mDebug(__func__,this),
    mFile(file),
    mBackend(file.GetBackend()),
    mCacheMgr(mBackend.GetCacheManager()),
    mPageSize(pageSize), 
    mFileSize(fileSize), 
    mBandwidth(__func__, mBackend.GetOptions().readAheadTime),
    mPageBackend(pageBackend)
{ 
    MDBG_INFO("(file:" << &file << ", size:" << fileSize << ", pageSize:" << pageSize << ")");
}

/*****************************************************/
PageManager::~PageManager()
{
    MDBG_INFO("() waiting for lock");

    mScopeMutex.lock(); // exclusive, forever

    if (mCacheMgr != nullptr)
    {
        for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); ++it)
            mCacheMgr->RemovePage(it->second);
    }

    MDBG_INFO("... returning!");
}

/*****************************************************/
void PageManager::ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLock& dataLock)
{
    MDBG_INFO("(" << mFile.GetName(dataLock) << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")");

    if (index*mPageSize + offset+length > mFileSize)
        throw std::invalid_argument(__func__); // fatal

    const Page& page { GetPageRead(index, dataLock) };

    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    const char* pageBuf { page.data() };
    std::copy(pageBuf+iOffset, pageBuf+iOffset+iLength, buffer); 
}

/*****************************************************/
void PageManager::WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName(dataLock) << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")");

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const uint64_t newFileSize = std::max(mFileSize, pageStart+offset+length);

    const size_t pageSize { min64st(newFileSize-pageStart, mPageSize) };
    const bool partial { offset > 0 || length < pageSize };

    MDBG_INFO("... pageSize:" << pageSize << " newFileSize:" << newFileSize);

    Page& page { GetPageWrite(index, pageSize, partial, dataLock) };
    mFileSize = newFileSize; // extend file
    page.mDirty = true;

    const auto iOffset { static_cast<decltype(Page::mData)::iterator::difference_type>(offset) };
    const auto iLength { static_cast<decltype(Page::mData)::iterator::difference_type>(length) };

    char* pageBuf { page.data() };
    std::copy(buffer, buffer+iLength, pageBuf+iOffset);
}

/*****************************************************/
const Page& PageManager::GetPageRead(const uint64_t index, const SharedLock& dataLock)
{
    MDBG_INFO("(" << mFile.GetName(dataLock) << ")" << " (index:" << index << ")");

    if (index*mPageSize >= mFileSize) throw std::invalid_argument(__func__); // fatal

    UniqueLock pagesLock(mPagesMutex);

    { PageMap::const_iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        DoAdvanceRead(index, dataLock, pagesLock);

        MDBG_INFO("... return existing page");
        const Page& page { it->second };
        
        if (mCacheMgr && !mBackend.isMemory()) 
            mCacheMgr->InformPage(*this, index, page, page.mDirty);
        return page;
    } }

    if (!isFetchPending(index, pagesLock))
    {
        const size_t fetchSize { GetFetchSize(index, dataLock, pagesLock) };
        if (!fetchSize) // bigger than backend, create empty
        {
            MDBG_INFO("... create empty page");
            Page& newPage { mPages.emplace(index, 0).first->second };

            ResizePage(newPage, mPageSize, false); // zeroize
            // hold pagesLock because if inform fails, we will remove this page
            // InformPage() is never synchronous without a dataLockW so holding pagesLock is okay
            InformNewPage(index, newPage, false, true, &pagesLock, nullptr);
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
    const Page& page { it->second };

    if (mCacheMgr && !mBackend.isMemory()) 
        mCacheMgr->InformPage(*this, index, page, page.mDirty);
    return page;
}

/*****************************************************/
Page& PageManager::GetPageWrite(const uint64_t index, const size_t pageSize, const bool partial, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName(dataLock) << ")" << " (index:" << index << " pageSize:" << pageSize << " partial:" << partial << ")");

    // don't need pagesLock, have an exclusive dataLock... also holding pagesLock
    // could lead to deadlock since InformNewPage() may synchronously evict/flush

    // mDirty is set AFTER informing the CacheManager as it may cause a synchronous flush

    { PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        MDBG_INFO("... returning existing page");
        InformResizePage(index, it->second, true, pageSize, dataLock);
        return it->second;
    } }

    // as we have an exclusive dataLock, we know there are no background reads,
    // so the page is not already pending, and we can read synchronously

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    if (pageStart >= mPageBackend.GetBackendSize(dataLock) || !partial)
    {
        if (pageStart > mFileSize && mFileSize > 0)
        {
            // extend the old last page if necessary
            PageMap::iterator it { mPages.find((mFileSize-1)/mPageSize) };
            if (it != mPages.end()) ResizePage(it->second, mPageSize, true, &dataLock);
        }

        MDBG_INFO("... create empty page");
        Page& newPage { mPages.emplace(index, 0).first->second };
        ResizePage(newPage, pageSize, false); // zeroize
        InformNewPage(index, newPage, true, true, nullptr, &dataLock);
        return newPage;
    }

    MDBG_INFO("... partial write, reading single");
    Page* newPage; mPageBackend.FetchPages(index, 1, // read a single page
        [&](const uint64_t pageIndex, const uint64_t pageStartI, const size_t pageSizeI, Page& page)
    {
        newPage = &(mPages.emplace(pageIndex, std::move(page)).first->second);
    }, dataLock);

    ResizePage(*newPage, pageSize, false);
    InformNewPage(index, *newPage, true, true, nullptr, &dataLock);
    MDBG_INFO("... returning pended page " << index);
    return *newPage;
}

/*****************************************************/
void PageManager::InformNewPage(const uint64_t index, Page& page, bool dirty, bool canWait, const UniqueLock* pagesLock, const SharedLockW* dataLock)
{
    assert(pagesLock != nullptr || dataLock != nullptr);

    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, dirty, canWait, dataLock); }
    catch (BaseException& ex)
    {
        mCacheMgr->RemovePage(page);
        mPages.erase(index); // undo memory usage
        std::rethrow_exception(std::current_exception());
    }
}

/*****************************************************/
void PageManager::InformResizePage(const uint64_t index, Page& page, bool dirty, const size_t pageSize, const SharedLockW& dataLock)
{
    const size_t oldSize { page.size() };
    ResizePage(page, pageSize, false);

    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, dirty, true, &dataLock); }
    catch (BaseException& ex)
    {
        mCacheMgr->ResizePage(*this, page, oldSize, &dataLock);
        ResizePage(page, oldSize, false); // undo memory usage
        std::rethrow_exception(std::current_exception());
    }
}

/*****************************************************/
void PageManager::ResizePage(Page& page, const size_t pageSize, bool cacheMgr, const SharedLockW* dataLock)
{
    const size_t oldSize { page.size() };
    if (oldSize == pageSize) return; // no-op

    MDBG_INFO("(pageSize:" << pageSize << ") oldSize:" << oldSize);

    // inform the cache manager before resizing in case of an error
    if (cacheMgr && mCacheMgr) 
        mCacheMgr->ResizePage(*this, page, pageSize, dataLock);

    page.mData.resize(pageSize);

    if (pageSize > oldSize) std::memset(
        page.data()+oldSize, 0, pageSize-oldSize);
}

/*****************************************************/
bool PageManager::isDirty(const uint64_t index, const SharedLockW& dataLock)
{
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
size_t PageManager::GetFetchSize(const uint64_t index, const SharedLock& dataLock, const UniqueLock& pagesLock)
{
    if (index*mPageSize >= mFileSize)
        { MDBG_ERROR("() ERROR index:" << index << " mFileSize:" << mFileSize 
            << " mPageSize:" << mPageSize); assert(false); return 0; }

    const uint64_t backendSize { mPageBackend.GetBackendSize(dataLock) };

    if (!backendSize) return 0;
    const uint64_t lastPage { (backendSize-1)/mPageSize }; // last valid page index
    if (index > lastPage) return 0; // can't read beyond the backend
    const size_t maxReadCount { min64st(lastPage-index+1, mFetchSize) };

    MDBG_INFO("(index:" << index << ") backendSize:" << backendSize << " maxReadCount:" << maxReadCount);

    if (mPages.find(index) != mPages.end()) return 0; // page exists

    // no read-ahead for the first page as file managers often read just metadata
    size_t readCount { (index > 0) ? maxReadCount : 1 };

    // stop before the next existing (pages are in order)
    { PageMap::const_iterator nextIt { mPages.upper_bound(index) };
    if (nextIt != mPages.end())
    {
        MDBG_INFO("... first page at:" << nextIt->first);
        readCount = min64st(nextIt->first-index, readCount);
    } }

    // stop before the next pending
    for (size_t curCount { 0 }; curCount < readCount; ++curCount)
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
void PageManager::DoAdvanceRead(const uint64_t index, const SharedLock& dataLock, const UniqueLock& pagesLock)
{
    // always pre-populate mReadAheadPages ahead (except index 0)
    if (index > 0) for (uint64_t nextIdx { index+1 }; 
        nextIdx <= index + mBackend.GetOptions().readAheadBuffer; ++nextIdx)
    {
        if (nextIdx*mPageSize >= mFileSize) break; // exit loop
        const size_t fetchSize { GetFetchSize(nextIdx, dataLock, pagesLock) };
        if (fetchSize)
        {
            MDBG_INFO("... read-ahead nextIdx:" << nextIdx << " fetchSize:" << fetchSize);
            StartFetch(nextIdx, fetchSize, pagesLock);
            break; // exit loop
        }
    }
}

/*****************************************************/
void PageManager::StartFetch(const uint64_t index, const size_t readCount, const UniqueLock& pagesLock)
{
    MDBG_INFO("(index:" << index << ", readCount:" << readCount << ")");

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
    SharedLockRP dataLock(GetReadPriLock());

    std::chrono::steady_clock::time_point timeStart { std::chrono::steady_clock::now() };

    uint64_t nextIndex { index }; try
    {
        const size_t readSize { mPageBackend.FetchPages(index, count, 
            [&](const uint64_t pageIndex, const uint64_t pageStart, const size_t pageSize, Page& page)
        {
            // if we are reading a page that is smaller on the backend (dirty writes), might need to extend
            const size_t realSize { min64st(mFileSize-pageStart, mPageSize) };
            if (pageSize < realSize) ResizePage(page, realSize, false);

            UniqueLock pagesLock(mPagesMutex);
            // hold pagesLock because if inform fails, we will remove this page
            PageMap::iterator newIt { mPages.emplace(pageIndex, std::move(page)).first };

            InformNewPage(pageIndex, newIt->second, false, false, &pagesLock, nullptr);
            // pass false to not wait - not allowed to call the backend for evict/flush within this callback
            // even if canWait was true, the CacheManager could have us skip the wait to get our W lock for evict
            RemovePendingFetch(pageIndex, true, pagesLock); 

            ++nextIndex;
        }, dataLock) };

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
        const uint64_t cacheMax { mCacheMgr->GetMemoryLimit()/mBackend.GetOptions().readMaxCacheFrac };
        if (targetBytes > cacheMax) // no point in downloading just to get evicted
        {
            targetBytes = cacheMax;
            MDBG_INFO("... cache limited targetBytes:" << cacheMax);
        }
    }
    
    mFetchSize = std::max(static_cast<size_t>(1), // between 1 and size_t:max
        min64st(targetBytes/mPageSize, std::numeric_limits<size_t>::max()));
    MDBG_INFO("... newFetchSize:" << mFetchSize);
}

/*****************************************************/
uint64_t PageManager::GetWriteList(PageMap::iterator& pageIt, PageBackend::PagePtrList& writeList, const SharedLockW& dataLock)
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
void PageManager::EvictPage(const uint64_t index, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName(dataLock) << ") (index:" << index << ")");

    // don't need mPagesMutex, have dataLockW

    PageMap::iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end())
    {
        if (pageIt->second.mDirty)
        {
            MDBG_INFO("... page is dirty, writing");
            PageBackend::PagePtrList writeList;
            PageMap::iterator pageItTmp { pageIt }; // copy

            GetWriteList(pageItTmp, writeList, dataLock);
            FlushPageList(index, writeList, dataLock);
        }

        if (mCacheMgr) mCacheMgr->RemovePage(pageIt->second);
        mPages.erase(pageIt); // need dataLockW for this

        MDBG_INFO("... page removed, numPages:" << mPages.size());
    }
    else { MDBG_INFO(" ... page not found"); }
}

/*****************************************************/
size_t PageManager::FlushPage(const uint64_t index, const SharedLockW& dataLock)
{
    MDBG_INFO("(" << mFile.GetName(dataLock) << ") (index:" << index << ")");

    PageBackend::PagePtrList writeList;

    PageMap::iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end() && pageIt->second.mDirty)
        GetWriteList(pageIt, writeList, dataLock);
    else { MDBG_INFO(" ... page not found"); }
    
    // flush pages first as this may handle truncating
    size_t written { FlushPageList(index, writeList, dataLock) };

    MDBG_INFO("... return:" << written); return written;
}

/*****************************************************/
void PageManager::FlushPages(const SharedLockW& dataLock)
{
    MDBG_INFO("()");

    // create runs of pages to write separate from mPages so we don't have to hold pagesLock
    std::map<uint64_t, PageBackend::PagePtrList> writeLists;

    PageMap::iterator pageIt { mPages.begin() };
    while (pageIt != mPages.end())
    {
        PageBackend::PagePtrList writeList;
        const uint64_t startIndex {
            GetWriteList(pageIt, writeList, dataLock) };

        if (!writeList.empty())
            writeLists.emplace(startIndex, std::move(writeList));
        // this loop is not N^2 as GetWriteList() increments our pageIt
    }

    MDBG_INFO("... write runs:" << writeLists.size());
    
    if (!writeLists.size()) // run anyway so FlushCreate() is called
        FlushPageList(0, PageBackend::PagePtrList(), dataLock);
    else for (const decltype(writeLists)::value_type& writePair : writeLists)
        FlushPageList(writePair.first, writePair.second, dataLock);

    MDBG_INFO("... returning!");
}

/*****************************************************/
size_t PageManager::FlushPageList(const uint64_t index, const PageBackend::PagePtrList& pages, const SharedLockW& dataLock)
{
    MDBG_INFO("(index:" << index << " pages:" << pages.size() << ")");

    // truncate is only cached before mBackendExists
    const bool flushTruncate { !mPageBackend.ExistsOnBackend(dataLock) };

    const size_t totalSize { pages.empty() ? 0 : 
        mPageBackend.FlushPageList(index, pages, dataLock) };

    for (Page* pagePtr : pages)
    {
        pagePtr->mDirty = false;
        if (mCacheMgr) mCacheMgr->RemoveDirty(*pagePtr);
    }

    if (flushTruncate) FlushTruncate(dataLock); // also calls FlushCreate()

    return totalSize;
}

/*****************************************************/
void PageManager::FlushTruncate(const SharedLockW& dataLock)
{
    MDBG_INFO("()");

    mPageBackend.FlushCreate(dataLock); // maybe not done yet

    uint64_t maxDirty { 0 }; // byte after last dirty byte
    for (PageMap::const_iterator it { mPages.begin() }; it != mPages.end(); ++it)
    {
        if (it->second.mDirty)
        {
            uint64_t pageMax { it->first*mPageSize + it->second.size() };
            maxDirty = std::max(maxDirty, pageMax);
        }
    }

    // mFileSize should only be larger than mBackendSize if dirty writes
    if (std::max(mPageBackend.GetBackendSize(dataLock), maxDirty) < mFileSize)
    {
        MDBG_INFO("... truncate maxDirty:" << maxDirty << " fileSize:" << mFileSize);
        mPageBackend.Truncate(mFileSize, dataLock);
    }
}

/*****************************************************/
void PageManager::RemoteChanged(const uint64_t backendSize, const SharedLockW& dataLock)
{
    if (!mPageBackend.ExistsOnBackend(dataLock)) return; // called Refresh() ourselves

    MDBG_INFO("(newSize:" << backendSize << ") oldSize:" 
        << mPageBackend.GetBackendSize(dataLock) << ")");

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
    mPageBackend.SetBackendSize(backendSize, dataLock);
}

/*****************************************************/
void PageManager::Truncate(const uint64_t newSize, const SharedLockW& dataLock)
{
    MDBG_INFO("(newSize:" << newSize << ")");

    mPageBackend.Truncate(newSize, dataLock);
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
