#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Folder.hpp"
#include "FSConfig.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/ConfigOptions.hpp"
using Andromeda::Backend::ConfigOptions;
#include "andromeda/filesystem/filedata/Page.hpp"
using Andromeda::Filesystem::Filedata::Page;
#include "andromeda/filesystem/filedata/PageManager.hpp"
using Andromeda::Filesystem::Filedata::PageManager;

namespace Andromeda {
namespace Filesystem {

// TODO consider changing this pattern so the constructor take only real params and then we have a static FromData()
// this way things can be actually initialized at construction and won't need Initialize()

/*****************************************************/
File::File(BackendImpl& backend, const nlohmann::json& data, Folder& parent) : 
    Item(backend), mDebug("File",this)
{
    mDebug << __func__ << "()"; mDebug.Info();

    Initialize(data); mParent = &parent;

    std::string fsid; try
    {
        data.at("size").get_to(mSize);
        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(mBackend, fsid);

    const size_t fsChunk { mFsConfig->GetChunkSize() };
    const size_t cfChunk { mBackend.GetOptions().pageSize };

    auto ceil { [](auto x, auto y) { return (x + y - 1) / y; } };
    const size_t pageSize { fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk };
    mPageManager = std::make_unique<PageManager>(*this, backend, pageSize);

    mDebug << __func__ << "... ID:" << GetID() << " name:" << mName 
        << " fsChunk:" << fsChunk << " cfChunk:" << cfChunk; mDebug.Info();
}

/*****************************************************/
File::~File() { } // for unique_ptr

/*****************************************************/
void File::Refresh(const nlohmann::json& data)
{
    Item::Refresh(data);

    try
    {
        decltype(mSize) newSize;
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
    mDebug << __func__ << "() " << GetID(); mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.DeleteFile(GetID());

    mDeleted = true; // TODO once threading works better this should not be required, see how pages use locks
}

/*****************************************************/
void File::SubRename(const std::string& newName, bool overwrite)
{
    mDebug << __func__ << "() " << GetID() << " (name:" << newName << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.RenameFile(GetID(), newName, overwrite);
}

/*****************************************************/
void File::SubMove(Folder& newParent, bool overwrite)
{
    mDebug << __func__ << "() " << GetID() << " (parent:" << newParent.GetName() << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

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

    mDebug << __func__ << "() " << GetID(); mDebug.Info();

    mPageManager->FlushPages(nothrow);
}

/*****************************************************/
void File::ReadPage(std::byte* buffer, const uint64_t index, const size_t offset, const size_t length)
{
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    const std::byte* pageBuf { mPageManager->GetPageRead(index) };

    const auto iOffset { static_cast<Page::Data::iterator::difference_type>(offset) };
    const auto iLength { static_cast<Page::Data::iterator::difference_type>(length) };

    std::copy(pageBuf+iOffset, pageBuf+iOffset+iLength, buffer); 
}

/*****************************************************/
void File::WritePage(const std::byte* buffer, const uint64_t index, const size_t offset, const size_t length)
{
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    std::byte* pageBuf { mPageManager->GetPageWrite(index, offset, length) };
    
    const auto iOffset { static_cast<Page::Data::iterator::difference_type>(offset) };
    const auto iLength { static_cast<Page::Data::iterator::difference_type>(length) };

    std::copy(buffer, buffer+iLength, pageBuf+iOffset);

    mSize = mPageManager->GetFileSize();
}

/*****************************************************/
size_t File::ReadBytes(std::byte* buffer, const uint64_t offset, size_t length)
{    
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (offset:" << offset 
        << " length:" << length << ") mSize:" << mSize; mDebug.Info(); }

    if (offset >= mSize) return 0;

    PageManager::SharedLockR lock { mPageManager->GetReadLock() };

    length = Filedata::min64st(mSize-offset, length);

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        const std::string data(mBackend.ReadFile(GetID(), offset, length));

        std::copy(data.cbegin(), data.cend(), reinterpret_cast<char*>(buffer));
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        const size_t pageSize { mPageManager->GetPageSize() };

        const uint64_t index { byte / pageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*pageSize) }; // offset within the page
        const size_t pLength { Filedata::min64st(length+offset-byte, pageSize-pOffset) }; // length within the page

        if (mDebug) { mDebug << __func__ << "... size:" << mSize << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; mDebug.Info(); }

        ReadPage(buffer, index, pOffset, pLength);

        buffer += pLength; byte += pLength;
    }

    return length;
}

/*****************************************************/
void File::WriteBytes(const std::byte* buffer, const uint64_t offset, const size_t length)
{
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    if (isReadOnly()) throw ReadOnlyException();

    const FSConfig::WriteMode writeMode(GetWriteMode());
    if (writeMode == FSConfig::WriteMode::NONE) throw WriteTypeException();

    PageManager::SharedLockW lock { mPageManager->GetWriteLock() };

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        if (writeMode == FSConfig::WriteMode::APPEND 
            && offset != mSize) throw WriteTypeException();

        const std::string data(reinterpret_cast<const char*>(buffer), length);

        mBackend.WriteFile(GetID(), offset, data);

        mSize = std::max(mSize, offset+length);
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        const size_t pageSize { mPageManager->GetPageSize() };

        if (writeMode == FSConfig::WriteMode::APPEND)
        {
            // allowed if (==fileSize and startOfPage) OR (within dirty page)
            if (!(offset == mSize && offset % pageSize == 0) && 
                !mPageManager->isDirty(offset/pageSize)) throw WriteTypeException();
        }

        const uint64_t index { byte / pageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*pageSize) }; // offset within the page
        const size_t pLength { Filedata::min64st(length+offset-byte, pageSize-pOffset) }; // length within the page

        if (mDebug) { mDebug << __func__ << "... mSize:" << mSize << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; mDebug.Info(); }

        WritePage(buffer, index, pOffset, pLength);
        
        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
void File::Truncate(const uint64_t newSize)
{    
    mDebug << __func__ << "() " << GetID() << " (size:" << newSize << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    if (GetWriteMode() < FSConfig::WriteMode::RANDOM) throw WriteTypeException();

    mPageManager->Truncate(newSize);
    mSize = mPageManager->GetFileSize();
}

} // namespace Filesystem
} // namespace Andromeda
