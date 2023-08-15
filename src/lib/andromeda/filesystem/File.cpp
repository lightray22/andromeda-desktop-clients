#include <algorithm>
#include <utility>
#include "nlohmann/json.hpp"

#include "File.hpp"
#include "Folder.hpp"
#include "FSConfig.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/SharedMutex.hpp"
#include "andromeda/backend/BackendException.hpp"
using Andromeda::Backend::BackendException;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/filedata/PageBackend.hpp"
using Andromeda::Filesystem::Filedata::PageBackend;
#include "andromeda/filesystem/filedata/PageManager.hpp"
using Andromeda::Filesystem::Filedata::PageManager;

namespace Andromeda {
namespace Filesystem {

/*****************************************************/
File::File(BackendImpl& backend, const nlohmann::json& data, Folder& parent) : 
    Item(backend, data), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mParent = &parent;

    uint64_t fileSize = 0;
    std::string fsid; 
    try
    {
        data.at("size").get_to(fileSize);
        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(mBackend, fsid);

    MDBG_INFO("... ID:" << mId << " name:" << mName);

    const size_t pageSize { CalcPageSize() };
    mPageBackend = std::make_unique<PageBackend>(*this, mId, fileSize, pageSize);
    mPageManager = std::make_unique<PageManager>(*this, fileSize, pageSize, *mPageBackend);
}

/*****************************************************/
File::File(BackendImpl& backend, Folder& parent, const std::string& name, const FSConfig& fsConfig,
    const File::CreateFunc& createFunc, const File::UploadFunc& uploadFunc) : 
    Item(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mFsConfig = &fsConfig;
    mParent = &parent;
    mName = name;

    mCreated = static_cast<decltype(mCreated)>(std::time(nullptr)); // now

    MDBG_INFO("... ID:" << mId << " name:" << mName);

    const size_t pageSize { CalcPageSize() };
    mPageBackend = std::make_unique<PageBackend>(*this, mId, pageSize, createFunc, uploadFunc);
    mPageManager = std::make_unique<PageManager>(*this, 0, pageSize, *mPageBackend);
}

/*****************************************************/
size_t File::CalcPageSize() const
{
    const size_t fsChunk { mFsConfig->GetChunkSize() };
    const size_t cfChunk { mBackend.GetOptions().pageSize };

    auto ceil { [](auto x, auto y) { return (x + y - 1) / y; } };
    const size_t pageSize { fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk };

    MDBG_INFO("... fsChunk:" << fsChunk << " cfChunk:" << cfChunk << " pageSize:" << pageSize);

    return pageSize;
}

/*****************************************************/
File::~File() = default; // for unique_ptr

/*****************************************************/
uint64_t File::GetSize(const SharedLock& thisLock) const 
{ 
    return mPageManager->GetFileSize(thisLock);
}

/*****************************************************/
size_t File::GetPageSize() const
{
    return mPageManager->GetPageSize();
}

/*****************************************************/
bool File::ExistsOnBackend(const SharedLock& thisLock) const
{
    return mPageBackend->ExistsOnBackend(thisLock);
}

/*****************************************************/
void File::Refresh(const nlohmann::json& data, const SharedLockW& thisLock)
{
    Item::Refresh(data, thisLock);

    try
    {
        if (mId.empty()) data.at("id").get_to(mId);

        uint64_t newSize = 0;
        data.at("size").get_to(newSize);
        // TODO use server mtime once supported here to check for changing
        // will also need a mBackendTime in case of dirty writes
        if (newSize != mPageBackend->GetBackendSize(thisLock))
            mPageManager->RemoteChanged(newSize, thisLock);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void File::SubDelete(const DeleteLock& deleteLock)
{
    ITDBG_INFO("()")

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    if (ExistsOnBackend(GetWriteLock()))
        mBackend.DeleteFile(GetID());
}

/*****************************************************/
void File::SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(name:" << newName << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    if (ExistsOnBackend(thisLock))
        mBackend.RenameFile(GetID(), newName, overwrite);
}

/*****************************************************/
void File::SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(parent:" << parentID << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    if (!ExistsOnBackend(thisLock))
        FlushCache(thisLock); // createFunc would no longer be valid
    
    mBackend.MoveFile(GetID(), parentID, overwrite);
}

/*****************************************************/
FSConfig::WriteMode File::GetWriteMode() const
{
    FSConfig::WriteMode writeMode(GetFSConfig().GetWriteMode());

    if (!mBackend.GetConfig().canRandWrite())
        writeMode = std::min(writeMode, FSConfig::WriteMode::APPEND);

    return writeMode;
}

/*****************************************************/
void File::FlushCache(const SharedLockW& thisLock, bool nothrow)
{
    ITDBG_INFO("()");

    if (!nothrow) mPageManager->FlushPages(thisLock);
    else try { mPageManager->FlushPages(thisLock); } catch (const BackendException& e){
        ITDBG_ERROR("... ignoring error: " << e.what()); }
}

/*****************************************************/
size_t File::ReadBytesMax(char* buffer, const uint64_t offset, const size_t maxLength, const SharedLock& thisLock)
{    
    ITDBG_INFO("(offset:" << offset << " maxLength:" << maxLength << ")");

    const uint64_t fileSize { mPageManager->GetFileSize(thisLock) };
    ITDBG_INFO("... fileSize:" << fileSize);

    if (offset >= fileSize) return 0;
    const size_t length { Filedata::min64st(fileSize-offset, maxLength) };

    ReadBytes(buffer, offset, length, thisLock);

    return length;
}

/*****************************************************/
void File::ReadBytes(char* buffer, const uint64_t offset, const size_t length, const SharedLock& thisLock)
{
    ITDBG_INFO("(offset:" << offset << " length:" << length << ")");

    if (offset + length > mPageManager->GetFileSize(thisLock))
        throw ReadBoundsException();

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        const std::string data { mBackend.ReadFile(GetID(), offset, length) };

        std::copy(data.cbegin(), data.cend(), buffer);
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        const size_t pageSize { mPageManager->GetPageSize() };

        const uint64_t index { byte / pageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*pageSize) }; // offset within the page
        const size_t pLength { Filedata::min64st(length+offset-byte, pageSize-pOffset) }; // length within the page

        ITDBG_INFO("... byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength);

        mPageManager->ReadPage(buffer, index, pOffset, pLength, thisLock);
        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
void File::WriteBytes(const char* buffer, uint64_t offset, size_t length, const SharedLockW& thisLock)
{
    ITDBG_INFO("(offset:" << offset << " length:" << length << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();
    const FSConfig::WriteMode writeMode(GetWriteMode());

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        const uint64_t fileSize { mPageManager->GetFileSize(thisLock) };

        // UPLOAD not allowed, APPEND only if offset == fileSize
        if (writeMode == FSConfig::WriteMode::UPLOAD
            || (writeMode == FSConfig::WriteMode::APPEND && offset != fileSize))
                throw WriteTypeException();

        const std::string data(buffer, length);
        mBackend.WriteFile(GetID(), offset, data);
        mPageManager->RemoteChanged(std::max(fileSize, offset+length), thisLock);
        return; // early return
    }
    
    if (writeMode == FSConfig::WriteMode::UPLOAD)
    {
        if (mPageBackend->ExistsOnBackend(thisLock)) 
            throw WriteTypeException();

        const uint64_t uploadMax { mBackend.GetConfig().GetUploadMaxBytes() };
        if (uploadMax && offset+length > uploadMax)
        {
            ITDBG_INFO("write exceeds upload max:" << uploadMax);
            throw BackendImpl::WriteSizeException(); // fail now rather than later
        }
    }
    else if (writeMode == FSConfig::WriteMode::APPEND)
    {
        if (mPageBackend->ExistsOnBackend(thisLock))
        {
            if (offset < mPageBackend->GetBackendSize(thisLock)) 
                throw WriteTypeException();

            // the PageManager can only upload at page boundaries
            const size_t fromBuffer { FixPageAlignment(buffer, offset, length, thisLock) };
            buffer += fromBuffer; offset += fromBuffer; length -= fromBuffer;
        }
    }

    // can't write starting after the end of the file w/o RANDOM
    if (writeMode < FSConfig::WriteMode::RANDOM)
        FillWriteHole(offset, thisLock);

    for (uint64_t byte { offset }; byte < offset+length; )
    {
        const size_t pageSize { mPageManager->GetPageSize() };

        const uint64_t index { byte / pageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*pageSize) }; // offset within the page
        const size_t pLength { Filedata::min64st(length+offset-byte, pageSize-pOffset) }; // length within the page

        ITDBG_INFO("... byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength);

        mPageManager->WritePage(buffer, index, pOffset, pLength, thisLock);
        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
size_t File::FixPageAlignment(const char* buffer, const uint64_t offset, const size_t length, const SharedLockW& thisLock)
{
    const uint64_t backendSize { mPageBackend->GetBackendSize(thisLock) };
    const size_t pageSize { mPageManager->GetPageSize() };
    const size_t pageError { static_cast<size_t>(backendSize % pageSize) };
    
    size_t fromBuffer { 0 }; if (pageError != 0)
    {
        // the data string is some zeroes before the buffer, then some data from the buffer
        const size_t writeSize { Filedata::min64st(offset+length-backendSize, pageSize-pageError) };
        const size_t fromZero { Filedata::min64st(offset-backendSize, writeSize) };
        fromBuffer = (fromZero < writeSize) ? writeSize-fromZero : 0;

        ITDBG_INFO("... write:" << writeSize << "@" << backendSize
            << ", fromZero:" << fromZero << " fromBuffer:" << fromBuffer);

        std::string data(fromZero, '\0');
        data += std::string(buffer, fromBuffer);
        
        mBackend.WriteFile(GetID(), backendSize, data);
        mPageManager->RemoteChanged(backendSize+writeSize, thisLock);
    }
    return fromBuffer;
}

/*****************************************************/
void File::FillWriteHole(const uint64_t offset, const SharedLockW& thisLock)
{
    const uint64_t fileSize { mPageManager->GetFileSize(thisLock) };
    if (offset > fileSize) // need to fill in holes to guarantee sequential upload
    {
        std::vector<char> holeBuf(GetPageSize(), 0); // chunked fill to avoid consuming too much memory
        for (uint64_t holeOffset = fileSize; holeOffset < offset; holeOffset += holeBuf.size())
        {
            const size_t holeSize { Filedata::min64st(offset-holeOffset, holeBuf.size()) };
            ITDBG_INFO("... fill write hole:" << holeSize << "@" << holeOffset);
            WriteBytes(holeBuf.data(), holeOffset, holeSize, thisLock);
        }
    }
}

/*****************************************************/
void File::Truncate(const uint64_t newSize, const SharedLockW& thisLock)
{    
    ITDBG_INFO("(size:" << newSize << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    const FSConfig::WriteMode writeMode(GetWriteMode());

    if (writeMode == FSConfig::WriteMode::UPLOAD
        || (writeMode == FSConfig::WriteMode::APPEND
            && newSize != 0)) throw WriteTypeException();

    mPageManager->Truncate(newSize, thisLock);
}

} // namespace Filesystem
} // namespace Andromeda
