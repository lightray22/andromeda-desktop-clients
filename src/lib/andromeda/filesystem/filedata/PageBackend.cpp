
#include <cassert>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>

#include "PageBackend.hpp"

#include "andromeda/Semaphor.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/RunnerInput.hpp"
using Andromeda::Backend::WriteFunc;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
PageBackend::PageBackend(File& file, const std::string& fileID, uint64_t backendSize, const size_t pageSize) :
    mPageSize(pageSize),
    mBackendSize(backendSize),
    mBackendExists(true),
    mFile(file),
    mFileID(fileID),
    mBackend(file.GetBackend()),
    mDebug(__func__,this) { }

/*****************************************************/
PageBackend::PageBackend(File& file, const std::string& fileID, const size_t pageSize, 
    const File::CreateFunc& createFunc, const File::UploadFunc& uploadFunc) :
    mPageSize(pageSize),
    mBackendSize(0),
    mBackendExists(false),
    mCreateFunc(createFunc),
    mUploadFunc(uploadFunc),
    mFile(file),
    mFileID(fileID),
    mBackend(file.GetBackend()),
    mDebug(__func__,this) { }

/*****************************************************/
size_t PageBackend::FetchPages(const uint64_t index, const size_t count, 
    const PageBackend::PageHandler& pageHandler, const SharedLock& thisLock)
{
    MDBG_INFO("(index:" << index << " count:" << count << ")");

    if (!count || (index+count-1)*mPageSize >= mBackendSize) 
        { MDBG_ERROR("() ERROR invalid index:" << index << " count:" << count 
            << " mBackendSize:" << mBackendSize << " mPageSize:" << mPageSize); assert(false); }

    const uint64_t pageStart { index*mPageSize }; // offset of the page start
    const size_t readSize { min64st(mBackendSize-pageStart, mPageSize*count) }; // length of data to fetch

    MDBG_INFO("... pageStart:" << pageStart << " readSize:" << readSize);

    uint64_t curIndex { index };
    std::unique_ptr<Page> curPage;

    mBackend.ReadFile(mFileID, pageStart, readSize, 
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

    if (curPage != nullptr) { MDBG_ERROR("() ERROR unfinished read!"); assert(false); }

    return readSize;
}

/*****************************************************/
size_t PageBackend::FlushPageList(const uint64_t index, const PageBackend::PagePtrList& pages, const SharedLockW& thisLock)
{
    MDBG_INFO("(index:" << index << " pages:" << pages.size() << ")");

    if (!pages.size()) { MDBG_ERROR("() ERROR empty list!"); assert(false); return 0; }

    size_t totalSize { 0 };
    for (const Page* pagePtr : pages)
        totalSize += pagePtr->size();

    const uint64_t writeStart { index*mPageSize };
    MDBG_INFO("... WRITING " << totalSize << " to " << writeStart);

    const FSConfig::WriteMode writeMode { mFile.GetWriteMode() };
    if (writeMode == FSConfig::WriteMode::UPLOAD && mBackendExists)
        { MDBG_ERROR("... invalid write for UPLOAD!"); assert(false); }
    else if (writeMode == FSConfig::WriteMode::APPEND && mBackendExists && writeStart != mBackendSize)
        { MDBG_ERROR("... invalid write for APPEND!"); assert(false); }

    if (!mBackendExists && index != 0)
    {
        if (writeMode < FSConfig::WriteMode::RANDOM)
            { MDBG_ERROR("... invalid write without RANDOM!"); assert(false); }
        FlushCreate(thisLock); // can't use Upload() w/o first page
    }

    WriteFunc writeFunc { [&](const size_t offset, char* const buf, const size_t buflen, size_t& read)->bool
    {
        const size_t pagesIdx { offset/mPageSize };
        if (pagesIdx >= pages.size()) return false;

        const Page& page { *pages[pagesIdx] };
        const size_t pageOffset { offset - pagesIdx*mPageSize };
        const size_t pageSize { page.mData.size() };
        if (pageOffset >= pageSize) return false;

        const char* copyData { page.mData.data()+pageOffset };
        read = std::min(pageSize-pageOffset,buflen);
        std::copy(copyData, copyData+read, buf); 
        return true; // initial check will catch when we're done
    }};

    if (!mBackendExists)
    {
        const bool oneshot { mFile.GetWriteMode() < FSConfig::WriteMode::APPEND };
        mFile.Refresh(mUploadFunc(mFile.GetName(thisLock),writeFunc,oneshot),thisLock);
        mBackendExists = true;
    }
    else mBackend.WriteFile(mFileID, writeStart, writeFunc);

    mBackendSize = std::max(mBackendSize, writeStart+totalSize);

    return totalSize;
}

/*****************************************************/
void PageBackend::FlushCreate(const SharedLockW& thisLock)
{
    MDBG_INFO("()");

    if (!mBackendExists)
    {
        mFile.Refresh(mCreateFunc(mFile.GetName(thisLock)),thisLock);
        mBackendExists = true;
    }
}

/*****************************************************/
void PageBackend::Truncate(const uint64_t newSize, const SharedLockW& thisLock)
{
    MDBG_INFO("(newSize:" << newSize << ")");

    if (mBackendExists)
    {
        mBackend.TruncateFile(mFileID, newSize); 
        mBackendSize = newSize;
    }
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
