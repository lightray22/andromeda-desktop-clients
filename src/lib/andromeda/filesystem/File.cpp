#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Folder.hpp"
#include "FSConfig.hpp"
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/SharedMutex.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/filedata/PageManager.hpp"
using Andromeda::Filesystem::Filedata::PageManager;

namespace Andromeda {
namespace Filesystem {

// TODO consider changing this pattern so the constructor take only real params and then we have a static FromData()
// this way things can be actually initialized at construction and won't need Initialize()

/*****************************************************/
File::File(BackendImpl& backend, const nlohmann::json& data, Folder& parent) : 
    Item(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

    Initialize(data); mParent = &parent;

    uint64_t fileSize;
    std::string fsid; 
    try
    {
        data.at("size").get_to(fileSize);
        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(mBackend, fsid);

    const size_t fsChunk { mFsConfig->GetChunkSize() };
    const size_t cfChunk { mBackend.GetOptions().pageSize };

    auto ceil { [](auto x, auto y) { return (x + y - 1) / y; } };
    const size_t pageSize { fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk };
    mPageManager = std::make_unique<PageManager>(*this, fileSize, pageSize, true);

    MDBG_INFO("... ID:" << GetID() << " name:" << mName 
        << " fsChunk:" << fsChunk << " cfChunk:" << cfChunk);
}

/*****************************************************/
File::File(BackendImpl& backend, Folder& parent, const std::string& name, const FSConfig& fsConfig) : 
    Item(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mFsConfig = &fsConfig;
    mParent = &parent;
    mName = name;

    mCreated = static_cast<decltype(mCreated)>(std::time(nullptr)); // now

    const size_t fsChunk { mFsConfig->GetChunkSize() };
    const size_t cfChunk { mBackend.GetOptions().pageSize };

    auto ceil { [](auto x, auto y) { return (x + y - 1) / y; } };
    const size_t pageSize { fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk };
    mPageManager = std::make_unique<PageManager>(*this, 0, pageSize, false);

    MDBG_INFO("... ID:" << GetID() << " name:" << mName 
        << " fsChunk:" << fsChunk << " cfChunk:" << cfChunk);
}

/*****************************************************/
File::~File() { } // for unique_ptr

/*****************************************************/
bool File::ExistsOnBackend() const
{
    return mPageManager->ExistsOnBackend();
}

/*****************************************************/
uint64_t File::GetSize() const 
{ 
    return mPageManager->GetFileSize();
}

/*****************************************************/
void File::Refresh(const nlohmann::json& data)
{
    Item::Refresh(data);

    try
    {
        if (mId.empty()) data.at("id").get_to(mId);

        uint64_t newSize;
        data.at("size").get_to(newSize);
        if (newSize != mPageManager->GetBackendSize())
            mPageManager->RemoteChanged(newSize);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void File::SubDelete()
{
    ITDBG_INFO("()")

    if (isReadOnly()) throw ReadOnlyException();

    if (mPageManager->ExistsOnBackend()) // TODO locking here??
        mBackend.DeleteFile(GetID());

    mDeleted = true; // TODO once threading works better this should not be required, see how pages use locks
}

/*****************************************************/
void File::SubRename(const std::string& newName, bool overwrite)
{
    ITDBG_INFO("(name:" << newName << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (mPageManager->ExistsOnBackend()) // TODO locking here??
        mBackend.RenameFile(GetID(), newName, overwrite);
}

/*****************************************************/
void File::SubMove(Folder& newParent, bool overwrite)
{
    ITDBG_INFO("(parent:" << newParent.GetName() << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (mPageManager->ExistsOnBackend()) // TODO locking here??
        mBackend.MoveFile(GetID(), newParent.GetID(), overwrite);
}

/*****************************************************/
FSConfig::WriteMode File::GetWriteMode() const
{
    FSConfig::WriteMode writeMode(GetFSConfig().GetWriteMode());

    if (writeMode >= FSConfig::WriteMode::RANDOM && 
        !mBackend.GetConfig().canRandWrite())
    {
        writeMode = FSConfig::WriteMode::APPEND;
    }

    return writeMode;
}

/*****************************************************/
void File::FlushCache(bool nothrow)
{
    if (mDeleted) return;

    ITDBG_INFO("()");

    if (!nothrow) mPageManager->FlushPages();
    else try { mPageManager->FlushPages(); } catch (BaseException& e){ 
        ITDBG_ERROR("... ignoring error: " << e.what()); }
}

/*****************************************************/
size_t File::ReadBytesMax(char* buffer, const uint64_t offset, const size_t maxLength)
{    
    ITDBG_INFO("(offset:" << offset << " maxLength:" << maxLength << ")");

    SharedLockR dataLock { mPageManager->GetReadLock() };

    const uint64_t fileSize { mPageManager->GetFileSize() };
    ITDBG_INFO("... fileSize:" << fileSize);

    if (offset >= fileSize) return 0;
    const size_t length { Filedata::min64st(fileSize-offset, maxLength) };

    ReadBytes(buffer, offset, length, dataLock);

    return length;
}

/*****************************************************/
void File::ReadBytes(char* buffer, const uint64_t offset, size_t length)
{    
    ITDBG_INFO("(offset:" << offset << " length:" << length << ")");

    SharedLockR dataLock { mPageManager->GetReadLock() };

    ReadBytes(buffer, offset, length, dataLock);
}

/*****************************************************/
void File::ReadBytes(char* buffer, const uint64_t offset, size_t length, const SharedLockR& dataLock)
{
    if (offset + length > mPageManager->GetFileSize())
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

        mPageManager->ReadPage(buffer, index, pOffset, pLength, dataLock);

        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
void File::WriteBytes(const char* buffer, const uint64_t offset, const size_t length)
{
    ITDBG_INFO("(offset:" << offset << " length:" << length << ")");

    if (isReadOnly()) throw ReadOnlyException();

    const FSConfig::WriteMode writeMode(GetWriteMode());
    if (writeMode == FSConfig::WriteMode::NONE) throw WriteTypeException();

    SharedLockW dataLock { mPageManager->GetWriteLock() };

    const uint64_t fileSize { mPageManager->GetFileSize() };
    ITDBG_INFO("... fileSize:" << fileSize);

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        if (writeMode == FSConfig::WriteMode::APPEND 
            && offset != mPageManager->GetFileSize()) throw WriteTypeException();

        const std::string data(buffer, length);
        mBackend.WriteFile(GetID(), offset, data);
        mPageManager->RemoteChanged(std::max(fileSize, offset+length));
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        const size_t pageSize { mPageManager->GetPageSize() };

        if (writeMode == FSConfig::WriteMode::APPEND)
        {
            // allowed if (==fileSize and startOfPage) OR (within dirty page)
            if (!(offset == fileSize && offset % pageSize == 0) && 
                !mPageManager->isDirty(offset/pageSize, dataLock)) throw WriteTypeException();
        }

        const uint64_t index { byte / pageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*pageSize) }; // offset within the page
        const size_t pLength { Filedata::min64st(length+offset-byte, pageSize-pOffset) }; // length within the page

        ITDBG_INFO("... byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength);

        mPageManager->WritePage(buffer, index, pOffset, pLength, dataLock);
        
        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
void File::Truncate(const uint64_t newSize)
{    
    ITDBG_INFO("(size:" << newSize << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (GetWriteMode() < FSConfig::WriteMode::RANDOM) throw WriteTypeException();

    mPageManager->Truncate(newSize);
}

} // namespace Filesystem
} // namespace Andromeda
