
#include <cassert>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>

#include "PageBackend.hpp"

#include "andromeda/Semaphor.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

// globally limit the maximum number of concurrent background ops
static Semaphor sBackendSem { 4 }; // TODO configurable later?

/*****************************************************/
PageBackend::PageBackend(File& file, const size_t pageSize, uint64_t backendSize, bool backendExists) :
    mPageSize(pageSize),
    mBackendSize(backendSize),
    mBackendExists(backendExists),
    mFile(file),
    mBackend(file.GetBackend()),
    mDebug(__func__,this) { }

/*****************************************************/
size_t PageBackend::FetchPages(const uint64_t index, const size_t count, 
    const PageBackend::PageHandler& pageHandler)
{
    SemaphorLock backendSem(sBackendSem); 
    MDBG_INFO("(index:" << index << " count:" << count << ")");

    if (!count || (index+count-1)*mPageSize >= mBackendSize) 
        throw std::invalid_argument(__func__);

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const size_t readSize { min64st(mBackendSize-pageStart, mPageSize*count) }; // length of data to fetch

    MDBG_INFO("... threads:" << sBackendSem.get_count() << 
        " pageStart:" << pageStart << " readSize:" << readSize);

    uint64_t curIndex { index };
    std::unique_ptr<Page> curPage;

    mBackend.ReadFile(mFile.GetID(), pageStart, readSize, 
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
                std::memcpy(pageBuf+pwOffset, rbuf, pwLength);

                if (pwOffset+pwLength == curPage->size()) // page is done
                {
                    pageHandler(curIndex, curPageStart, pageSize, *curPage);
                    curPage.reset(); ++curIndex;
                }
            }
            else { MDBG_INFO("... old read, ignoring"); }

            rbuf += pwLength; rbyte += pwLength;
        }
    });

    if (curPage != nullptr) { assert(false); MDBG_ERROR("() ERROR unfinished read!"); } // stop only in debug builds

    return readSize;
}

/*****************************************************/
size_t PageBackend::FlushPageList(const uint64_t index, const PageBackend::PagePtrList& pages)
{
    SemaphorLock backendSem(sBackendSem); 
    MDBG_INFO("(index:" << index << " pages:" << pages.size() << ")");

    if (!pages.size()) 
        throw std::invalid_argument(__func__);

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

    if (!mBackendExists && index != 0) // can't use Upload()
    {
        mFile.Refresh(mBackend.CreateFile(
             mFile.GetParent().GetID(), mFile.GetName()));
        mBackendExists = true;
    }

    if (!mBackendExists)
    {
        mFile.Refresh(mBackend.UploadFile(
            mFile.GetParent().GetID(), mFile.GetName(), buf));
        mBackendExists = true;
    }
    else mBackend.WriteFile(mFile.GetID(), writeStart, buf);

    mBackendSize = std::max(mBackendSize, writeStart+totalSize);

    return totalSize;
}

/*****************************************************/
void PageBackend::FlushCreate()
{
    MDBG_INFO("()");

    if (!mBackendExists)
    {
        mFile.Refresh(mBackend.CreateFile(
            mFile.GetParent().GetID(), mFile.GetName()));
        mBackendExists = true;
    }
}

/*****************************************************/
void PageBackend::Truncate(const uint64_t newSize)
{
    MDBG_INFO("(newSize:" << newSize << ")");

    if (mBackendExists)
    {
        mBackend.TruncateFile(mFile.GetID(), newSize); 
        mBackendSize = newSize;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
