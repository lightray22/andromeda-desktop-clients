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
using Andromeda::Filesystem::Filedata::PageReader;
using Andromeda::Filesystem::Filedata::PageWriter;
#include "andromeda/filesystem/filedata/PageManager.hpp"
using Andromeda::Filesystem::Filedata::PageManager;

namespace Andromeda {
namespace Filesystem {

/** return the size_t min of a (uint64_t and size_t) */
size_t min64st(uint64_t s1, size_t s2)
{
    return static_cast<size_t>(std::min(s1, static_cast<uint64_t>(s2)));
}

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
        mBackendSize = mSize;

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

    uint64_t newSize; try
    {
        data.at("size").get_to(newSize);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    if (newSize == mBackendSize) return;

    mDebug << __func__ << "... backend changed size!" << " old:" << mBackendSize 
        << " new:" << newSize << " size:" << mSize; mDebug.Info();

    mBackendSize = newSize; 
    uint64_t maxDirty { 0 };

    // get new max size = max(server size, dirty byte) and purge extra mPages
    /*for (PageMap::reverse_iterator it { mPages.rbegin() }; it != mPages.rend(); )
    {
        uint64_t pageStart { it->first * mPageSize };

        if (pageStart >= mBackendSize)
        {
            Page& page(it->second);

            // dirty mPages will extend the file again when written
            if (page.dirty) { maxDirty = std::min(mSize, pageStart + mPageSize); break; }

            // hacky reverse_iterator version of it = mPages.erase(it);
            else it = decltype(it)(mPages.erase(std::next(it).base())); // erase deleted page
        }
        else break; // future pageStarts are smaller
    }*/ // TODO fix writes

    mSize = std::max(mBackendSize, maxDirty);
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
    /*if (mDeleted) return; // TODO fix writes

    mDebug << mName << ":" << __func__ << "()"; mDebug.Info();

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); ++it)
    {
        const uint64_t index(it->first); Page& page(it->second);

        if (page.dirty)
        {
            const uint64_t pageOffset { index*mPageSize };
            const size_t pageSize { min64st(mSize-pageOffset, mPageSize) };

            std::string data(reinterpret_cast<const char*>(page.data.data()), pageSize);

            if (mDebug) { mDebug << __func__ << "... index:" << index << " offset:" << pageOffset << " size:" << pageSize; mDebug.Info(); }

            auto writeFunc { [&]()->void { mBackend.WriteFile(GetID(), pageOffset, data); } };

            if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const BaseException& e) { 
                mDebug << __func__ << "... Ignoring Error: " << e.what(); mDebug.Error(); }

            page.dirty = false; mBackendSize = std::max(mBackendSize, pageOffset+pageSize);
        }
    }*/
}

/*****************************************************/
void File::ReadPage(std::byte* buffer, const uint64_t index, const size_t offset, const size_t length)
{
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    const PageReader page(mPageManager->GetPageReader(index));

    const auto iOffset { static_cast<Page::Data::iterator::difference_type>(offset) };
    const auto iLength { static_cast<Page::Data::iterator::difference_type>(length) };

    std::copy(page->cbegin()+iOffset, page->cbegin()+iOffset+iLength, buffer);
}

/*****************************************************/
void File::WritePage(const std::byte* buffer, const uint64_t index, const size_t offset, const size_t length)
{
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    /*Page& page(GetPage(index)); page.dirty = true;

    if (page.data.size() < offset+length) 
        page.data.resize(offset+length);
        
    const auto iOffset { static_cast<Page::Data::iterator::difference_type>(offset) };
    const auto iLength { static_cast<Page::Data::iterator::difference_type>(length) };

    std::copy(buffer, buffer+iLength, page.data.begin()+iOffset);*/ // TODO fix writes
}

/*****************************************************/
size_t File::ReadBytes(std::byte* buffer, const uint64_t offset, size_t length)
{    
    if (mDebug) { mDebug << __func__ << "() " << GetID() << " (offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    if (offset >= mSize) return 0;

    length = min64st(mSize-offset, length);

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        const std::string data(mBackend.ReadFile(GetID(), offset, length));

        std::copy(data.cbegin(), data.cend(), reinterpret_cast<char*>(buffer));
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        const size_t pageSize { mPageManager->GetPageSize() };

        const uint64_t index { byte / pageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*pageSize) };
        const size_t pLength { min64st(length+offset-byte, pageSize-pOffset) };

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

    /*if (isReadOnly()) throw ReadOnlyException();

    const FSConfig::WriteMode writeMode(GetWriteMode());
    if (writeMode == FSConfig::WriteMode::NONE) throw WriteTypeException();

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        if (writeMode == FSConfig::WriteMode::APPEND 
            && offset != mBackendSize) throw WriteTypeException();

        const std::string data(reinterpret_cast<const char*>(buffer), length);

        mBackend.WriteFile(GetID(), offset, data);

        mSize = std::max(mSize, offset+length); 
        mBackendSize = mSize;
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        if (writeMode == FSConfig::WriteMode::APPEND)
        {
            // allowed if (==fileSize and startOfPage) OR (within dirty page)
            if (!(offset == mSize && offset % mPageSize == 0) && 
                !(GetPage(offset/mPageSize).dirty)) throw WriteTypeException();
        }

        const uint64_t index { byte / mPageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*mPageSize) };
        const size_t pLength { min64st(length+offset-byte, mPageSize-pOffset) };

        if (mDebug) { mDebug << __func__ << "... size:" << mSize << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; mDebug.Info(); }

        WritePage(buffer, index, pOffset, pLength);   
    
        mSize = std::max(mSize, byte+pLength);
        
        buffer += pLength; byte += pLength;
    }*/ // TODO fix writes
}

/*****************************************************/
void File::Truncate(const uint64_t newSize)
{    
    mDebug << __func__ << "() " << GetID() << " (size:" << newSize << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    if (GetWriteMode() < FSConfig::WriteMode::RANDOM) throw WriteTypeException();

    mBackend.TruncateFile(GetID(), newSize); 

    mSize = newSize; mBackendSize = newSize;

    mPageManager->Truncate(newSize);
}

} // namespace Filesystem
} // namespace Andromeda
