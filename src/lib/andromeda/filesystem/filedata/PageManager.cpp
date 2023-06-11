
#include <cassert>
#include <cstring>
#include <limits>
#include <utility>

#include "CacheManager.hpp"
#include "Page.hpp"
#include "PageManager.hpp"
#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;
#include "andromeda/filesystem/Item.hpp"
using Andromeda::Filesystem::Item;

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

    Item::DeleteLock deleteLock(mScopeMutex); // exclusive

    if (mCacheMgr != nullptr)
    {
        for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); ++it)
            mCacheMgr->RemovePage(it->second);
    }

    MDBG_INFO("... returning!");
}

/*****************************************************/
void PageManager::ReadPage(char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLock& thisLock)
{
    MDBG_INFO("(" << mFile.GetName(thisLock) << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")");

    if (index*mPageSize + offset+length > mFileSize) { MDBG_ERROR("... invalid read!"); assert(false); }

    const Page& page { GetPageRead(index, thisLock) };

    std::memcpy(buffer, page.data()+offset, length);
}

/*****************************************************/
void PageManager::WritePage(const char* buffer, const uint64_t index, const size_t offset, const size_t length, const SharedLockW& thisLock)
{
    MDBG_INFO("(" << mFile.GetName(thisLock) << ")" << " (index:" << index << " offset:" << offset << " length:" << length << ")");

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const uint64_t newFileSize = std::max(mFileSize, pageStart+offset+length);

    const size_t pageSize { min64st(newFileSize-pageStart, mPageSize) };
    const bool partial { offset > 0 || length < pageSize };

    MDBG_INFO("... pageSize:" << pageSize << " newFileSize:" << newFileSize);

    // mDirty is set LAST since GetPageWrite() may cause a synchronous flush
    Page& page { GetPageWrite(index, pageSize, partial, thisLock) };
    mFileSize = newFileSize; // extend file
    page.setDirty();

    std::memcpy(page.data()+offset, buffer, length);
}

/*****************************************************/
const Page& PageManager::GetPageRead(const uint64_t index, const SharedLock& thisLock)
{
    MDBG_INFO("(" << mFile.GetName(thisLock) << ")" << " (index:" << index << ")");

    if (index*mPageSize >= mFileSize) { MDBG_ERROR("... invalid read!"); assert(false); }

    UniqueLock pagesLock(mPagesMutex);

    { PageMap::const_iterator it { mPages.find(index) };
    if (it != mPages.end()) 
    {
        DoAdvanceRead(index, thisLock, pagesLock);

        MDBG_INFO("... return existing page");
        const Page& page { it->second };
        
        if (mCacheMgr && !mBackend.isMemory()) 
            mCacheMgr->InformPage(*this, index, page, page.isDirty());
        return page;
    } }

    if (!isFetchPending(index, pagesLock))
    {
        const size_t fetchSize { GetFetchSize(index, thisLock, pagesLock) };
        if (!fetchSize) // bigger than backend, create empty
        {
            MDBG_INFO("... create empty page");
            Page& newPage { mPages.emplace(std::piecewise_construct, std::forward_as_tuple(index), 
                std::forward_as_tuple(0, mBackend.GetPageAllocator())).first->second };
            
            ResizePage(newPage, mPageSize, false); // zeroize
            // hold pagesLock because if inform fails, we will remove this page
            // use non-synchronous InformNewPage() so holding pagesLock is okay
            InformNewPage(index, newPage, false, true, pagesLock);
            return newPage;
        }
        else StartFetch(index, fetchSize, pagesLock);
    }

    PageMap::const_iterator it;
    std::exception_ptr fail;

    while ((it = mPages.find(index)) == mPages.end() &&
            !(fail = isFetchFailed(index, pagesLock)))
    {
        MDBG_INFO("... waiting for pending " << index);
        mPagesCV.wait(pagesLock);
    }

    if (fail != nullptr)
    {
        MDBG_INFO("... rethrowing exception");
        std::rethrow_exception(fail);
    }

    MDBG_INFO("... returning pended page " << index);
    const Page& page { it->second };

    if (mCacheMgr && !mBackend.isMemory()) 
        mCacheMgr->InformPage(*this, index, page, page.isDirty());
    return page;
}

/*****************************************************/
Page& PageManager::GetPageWrite(const uint64_t index, const size_t pageSize, const bool partial, const SharedLockW& thisLock)
{
    MDBG_INFO("(" << mFile.GetName(thisLock) << ")" << " (index:" << index << " pageSize:" << pageSize << " partial:" << partial << ")");

    { PageMap::iterator it = mPages.find(index);
    if (it != mPages.end())
    {
        MDBG_INFO("... returning existing page");
        InformResizePage(index, it->second, true, pageSize, thisLock);
        return it->second;
    } }

    // as we have an exclusive thisLock, we know there are no background reads,
    // so the page is not already pending, and we can read synchronously

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    if (pageStart >= mPageBackend.GetBackendSize(thisLock) || !partial)
    {
        if (pageStart > mFileSize && mFileSize > 0)
        {
            // extend the old last page if necessary
            PageMap::iterator it { mPages.find((mFileSize-1)/mPageSize) };
            if (it != mPages.end()) ResizePage(it->second, mPageSize, true, &thisLock);
        }

        MDBG_INFO("... create empty page");
        Page& newPage { mPages.emplace(std::piecewise_construct, std::forward_as_tuple(index), 
                std::forward_as_tuple(0, mBackend.GetPageAllocator())).first->second };
        ResizePage(newPage, pageSize, false); // zeroize
        InformNewPageSync(index, newPage, true, thisLock);
        return newPage;
    }

    MDBG_INFO("... partial write, reading single");
    Page* newPage; mPageBackend.FetchPages(index, 1, // read a single page
        [&](const uint64_t pageIndex, const uint64_t pageStartI, const size_t pageSizeI, Page&& page)
    {
        newPage = &(mPages.emplace(pageIndex, std::move(page)).first->second);
    }, thisLock);

    ResizePage(*newPage, pageSize, false);
    InformNewPageSync(index, *newPage, true, thisLock);
    MDBG_INFO("... returning pended page " << index);
    return *newPage;
}

/*****************************************************/
void PageManager::InformNewPage(const uint64_t index, const Page& page, bool dirty, bool canWait, const UniqueLock& pagesLock)
{
    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, dirty, canWait); }
    catch (BaseException& ex)
    {
        mCacheMgr->RemovePage(page);
        mPages.erase(index); // undo memory usage
        throw; // rethrow
    }
}

/*****************************************************/
void PageManager::InformNewPageSync(const uint64_t index, const Page& page, bool dirty, const SharedLockW& thisLock)
{
    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, dirty, true, &thisLock); }
    catch (BaseException& ex)
    {
        mCacheMgr->RemovePage(page);
        mPages.erase(index); // undo memory usage
        throw; // rethrow
    }
}

/*****************************************************/
void PageManager::InformResizePage(const uint64_t index, Page& page, bool dirty, const size_t pageSize, const SharedLockW& thisLock)
{
    const size_t oldSize { page.size() };
    ResizePage(page, pageSize, false);

    if (mCacheMgr && !mBackend.isMemory())
        try { mCacheMgr->InformPage(*this, index, page, dirty, true, &thisLock); }
    catch (BaseException& ex)
    {
        ResizePage(page, oldSize, false); // undo memory usage
        mCacheMgr->ResizePage(*this, page, &thisLock);
        throw; // rethrow
    }
}

/*****************************************************/
void PageManager::ResizePage(Page& page, const size_t pageSize, bool cacheMgr, const SharedLockW* thisLock)
{
    const size_t oldSize { page.size() };
    if (oldSize == pageSize) return; // no-op

    MDBG_INFO("(pageSize:" << pageSize << ") oldSize:" << oldSize);

    page.resize(pageSize);
    if (pageSize > oldSize) std::memset(
        page.data()+oldSize, 0, pageSize-oldSize);

    if (cacheMgr && mCacheMgr) 
        try { mCacheMgr->ResizePage(*this, page, thisLock); }
    catch (BaseException& ex)
    {
        page.resize(oldSize);
        mCacheMgr->ResizePage(*this, page, thisLock);
        throw; // rethrow
    }
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

    MDBG_ERROR("... page:" << index << " was not pending!");
}

/*****************************************************/
size_t PageManager::GetFetchSize(const uint64_t index, const SharedLock& thisLock, const UniqueLock& pagesLock)
{
    if (index*mPageSize >= mFileSize)
        { MDBG_ERROR("() ERROR index:" << index << " mFileSize:" << mFileSize 
            << " mPageSize:" << mPageSize); assert(false); return 0; }

    const uint64_t backendSize { mPageBackend.GetBackendSize(thisLock) };

    if (!backendSize) return 0;
    const uint64_t lastPage { (backendSize-1)/mPageSize }; // last valid page index
    if (index > lastPage) return 0; // can't read beyond the backend

    UniqueLock llock(mFetchSizeMutex);
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
void PageManager::DoAdvanceRead(const uint64_t index, const SharedLock& thisLock, const UniqueLock& pagesLock)
{
    // always pre-populate mReadAheadPages ahead (except index 0)
    if (index > 0) for (uint64_t nextIdx { index+1 }; 
        nextIdx <= index + mBackend.GetOptions().readAheadBuffer; ++nextIdx)
    {
        if (nextIdx*mPageSize >= mFileSize) break; // exit loop
        const size_t fetchSize { GetFetchSize(nextIdx, thisLock, pagesLock) };
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
    ScopeLocked scopeLock { TryLockScope() };
    if (!scopeLock) throw Folder::NotFoundException();

    // use a read-priority lock since the caller is waiting on us, 
    // if another write happens in the middle we would deadlock
    SharedLockRP thisLock { GetReadPriLock() };

    uint64_t curIndex { index }; try
    {
        MDBG_INFO("(index:" << index << " count:" << count << ")");
        std::chrono::steady_clock::time_point timeStart { std::chrono::steady_clock::now() };

        const size_t readSize { mPageBackend.FetchPages(index, count, 
            [&](const uint64_t pageIndex, const uint64_t pageStart, const size_t pageSize, Page&& page)
        {
            // if we are reading a page that is smaller on the backend (dirty writes), might need to extend
            const size_t realSize { min64st(mFileSize-pageStart, mPageSize) };
            if (pageSize < realSize) ResizePage(page, realSize, false);

            UniqueLock pagesLock(mPagesMutex);
            // hold pagesLock because if inform fails, we will remove this page
            PageMap::iterator newIt { mPages.emplace(pageIndex, std::move(page)).first };

            InformNewPage(pageIndex, newIt->second, false, false, pagesLock);
            // pass false to not wait - not allowed to call the backend for evict/flush within this callback
            // even if canWait was true, the CacheManager could have us skip the wait to get our W lock for evict
            RemovePendingFetch(pageIndex, true, pagesLock); 

            ++curIndex;
        }, thisLock) };

        if (readSize >= mPageSize) // don't consider small reads
            UpdateBandwidth(readSize, std::chrono::steady_clock::now()-timeStart);
    }
    catch (const BaseException& ex)
    {
        MDBG_ERROR("... " << ex.what());
        UniqueLock pagesLock(mPagesMutex);

        std::exception_ptr exptr { std::current_exception() };
        for (uint64_t eidx { curIndex }; eidx < curIndex+count; ++eidx)
            mFailedPages[eidx] = exptr; // shared_ptr

        RemovePendingFetch(curIndex, false, pagesLock);
    }
    
    MDBG_INFO("... thread returning!");
}

/*****************************************************/
void PageManager::UpdateBandwidth(const size_t bytes, const std::chrono::steady_clock::duration& time)
{
    UniqueLock llock(mFetchSizeMutex);

    size_t targetBytes { mBandwidth.UpdateBandwidth(bytes, time) };

    if (mCacheMgr)
    {
        const size_t cacheMax { mCacheMgr->GetMemoryLimit()/mBackend.GetOptions().readMaxCacheFrac };
        if (targetBytes > cacheMax) // no point in downloading just to get evicted
        {
            targetBytes = cacheMax;
            MDBG_INFO("... cache limited targetBytes:" << cacheMax);
        }
    }
    
    mFetchSize = std::max(static_cast<size_t>(1), targetBytes/mPageSize);
    MDBG_INFO("... newFetchSize:" << mFetchSize);
}

/*****************************************************/
uint64_t PageManager::GetWriteList(PageMap::iterator& pageIt, PageBackend::PagePtrList& writeList, const SharedLockW& thisLock)
{
    MDBG_INFO("(pageItIndex:" << pageIt->first << ")");

    size_t curSize { 0 };
    uint64_t lastIndex { 0 };

    uint64_t startIndex { (mFileSize-1)/mPageSize+1 }; // invalid

    for (; pageIt != mPages.end(); ++pageIt)
    {
        if (pageIt->second.isDirty())
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
void PageManager::EvictPage(const uint64_t index, const SharedLockW& thisLock)
{
    MDBG_INFO("(" << mFile.GetName(thisLock) << ") (index:" << index << ")");

    PageMap::iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end())
    {
        // ignore dirty evictions if we can't random write
        const bool randWrite { mFile.GetWriteMode() >= FSConfig::WriteMode::RANDOM };

        if (pageIt->second.isDirty() && randWrite)
        {
            MDBG_INFO("... page is dirty, writing");
            PageBackend::PagePtrList writeList;
            PageMap::iterator pageItTmp { pageIt }; // copy

            GetWriteList(pageItTmp, writeList, thisLock);
            // writeList won't be empty as the start page is dirty
            FlushPageList(index, writeList, thisLock);
        }

        if (mCacheMgr) mCacheMgr->RemovePage(pageIt->second);

        if (!pageIt->second.isDirty() || randWrite)
            mPages.erase(pageIt);
        else mDeferredEvicts.push_back(pageIt->first);

        MDBG_INFO("... page removed, numPages:" << mPages.size());
    }
    else { MDBG_INFO(" ... page not found"); }
}

/*****************************************************/
size_t PageManager::FlushPage(const uint64_t index, const SharedLockW& thisLock)
{
    MDBG_INFO("(" << mFile.GetName(thisLock) << ") (index:" << index << ")");

    size_t written { 0 };
    PageMap::iterator pageIt { mPages.find(index) };
    if (pageIt != mPages.end())
    {
        // ignore page flushes if we can't random write
        const bool randWrite { mFile.GetWriteMode() >= FSConfig::WriteMode::RANDOM };
        
        PageBackend::PagePtrList writeList;
        if (pageIt->second.isDirty() && randWrite)
            GetWriteList(pageIt, writeList, thisLock);
        else { MDBG_INFO(" ... page not found"); }
        
        if (writeList.size())
            written = FlushPageList(index, writeList, thisLock);
        else if (mCacheMgr) mCacheMgr->RemoveDirty(pageIt->second);
    }
    
    MDBG_INFO("... return:" << written); return written;
}

/*****************************************************/
void PageManager::FlushPages(const SharedLockW& thisLock)
{
    MDBG_INFO("()");

    // create runs of pages to write separate from mPages so we don't have to hold pagesLock
    std::map<uint64_t, PageBackend::PagePtrList> writeLists;

    PageMap::iterator pageIt { mPages.begin() };
    while (pageIt != mPages.end())
    {
        PageBackend::PagePtrList writeList;
        const uint64_t startIndex {
            GetWriteList(pageIt, writeList, thisLock) };

        if (!writeList.empty())
            writeLists.emplace(startIndex, std::move(writeList));
        // this loop is not N^2 as GetWriteList() increments our pageIt
    }

    MDBG_INFO("... write runs:" << writeLists.size());
    
    if (!writeLists.size()) // run anyway so FlushCreate() is called
        FlushPageList(0, PageBackend::PagePtrList(), thisLock);
    else for (const decltype(writeLists)::value_type& writePair : writeLists)
        FlushPageList(writePair.first, writePair.second, thisLock);

    for (const uint64_t pageIdx : mDeferredEvicts)
        EvictPage(pageIdx, thisLock);
    mDeferredEvicts.clear();

    MDBG_INFO("... returning!");
}

/*****************************************************/
size_t PageManager::FlushPageList(const uint64_t index, const PageBackend::PagePtrList& pages, const SharedLockW& thisLock)
{
    MDBG_INFO("(index:" << index << " pages:" << pages.size() << ")");

    // truncate is only cached before mBackendExists
    const bool flushCreate { !mPageBackend.ExistsOnBackend(thisLock) };

    const size_t totalSize { pages.empty() ? 0 : 
        mPageBackend.FlushPageList(index, pages, thisLock) };

    for (Page* pagePtr : pages)
    {
        pagePtr->setDirty(false);
        if (mCacheMgr) mCacheMgr->RemoveDirty(*pagePtr);
    }

    if (flushCreate) FlushCreate(thisLock); // also calls FlushCreate()

    return totalSize;
}

/*****************************************************/
void PageManager::FlushCreate(const SharedLockW& thisLock)
{
    MDBG_INFO("()");

    mPageBackend.FlushCreate(thisLock); // maybe not done yet

    uint64_t maxDirty { 0 }; // byte after last dirty byte
    for (PageMap::const_iterator it { mPages.begin() }; it != mPages.end(); ++it)
    {
        if (it->second.isDirty())
        {
            uint64_t pageMax { it->first*mPageSize + it->second.size() };
            maxDirty = std::max(maxDirty, pageMax);
        }
    }

    // mFileSize should only be larger than mBackendSize if dirty writes
    if (std::max(mPageBackend.GetBackendSize(thisLock), maxDirty) < mFileSize)
    {
        MDBG_INFO("... truncate maxDirty:" << maxDirty << " fileSize:" << mFileSize);
        mPageBackend.Truncate(mFileSize, thisLock);
    }
}

/*****************************************************/
void PageManager::RemoteChanged(const uint64_t backendSize, const SharedLockW& thisLock)
{
    if (!mPageBackend.ExistsOnBackend(thisLock)) return; // called Refresh() ourselves

    MDBG_INFO("(newSize:" << backendSize << ") oldSize:" 
        << mPageBackend.GetBackendSize(thisLock) << ")");

    uint64_t maxDirty { 0 };  // byte after last dirty byte
    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        const Page& page { it->second };
        if (!page.isDirty()) // evict all non-dirty
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
    mPageBackend.SetBackendSize(backendSize, thisLock);
}

/*****************************************************/
void PageManager::Truncate(const uint64_t newSize, const SharedLockW& thisLock)
{
    MDBG_INFO("(newSize:" << newSize << ")");

    mPageBackend.Truncate(newSize, thisLock);
    mFileSize = newSize;

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/mPageSize) // remove past end
        {
            MDBG_INFO("... erase page:" << it->first);

            if (mCacheMgr) mCacheMgr->RemovePage(it->second);
            it = mPages.erase(it);
        }
        else if (it->first == (newSize-1)/mPageSize) // resize new last page
        {
            const size_t pageSize { static_cast<size_t>(newSize - it->first*mPageSize) };
            MDBG_INFO("... resize new last page:" << it->first << " size:" << pageSize);
            ResizePage(it->second, pageSize, true, &thisLock); ++it;
        }
        else if (it->second.size() != mPageSize) // resize old last page
        {
            MDBG_INFO("... resize old last page:" << it->first << " size:" << mPageSize);
            ResizePage(it->second, mPageSize, true, &thisLock); ++it;
        }
        else ++it;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
