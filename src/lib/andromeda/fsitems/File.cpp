#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Folder.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/FSConfig.hpp"

namespace Andromeda {
namespace FSItems {

/** return the size_t min of a (uint64_t and size_t) */
size_t min64st(uint64_t s1, size_t s2)
{
    return static_cast<size_t>(std::min(s1, static_cast<uint64_t>(s2)));
}

/*****************************************************/
File::File(Backend& backend, const nlohmann::json& data, Folder& parent) : 
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
        throw Backend::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(mBackend, fsid);

    const size_t fsChunk { mFsConfig->GetChunkSize() };
    const size_t cfChunk { mBackend.GetConfig().GetOptions().pageSize };

    auto ceil { [](auto x, auto y) { return (x + y - 1) / y; } };
    mPageSize = fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk;

    mDebug << mName << ":" << __func__ << "... fsChunk:" << fsChunk << " cfChunk:" << cfChunk << " mPageSize:" << mPageSize; mDebug.Info();
}

/*****************************************************/
void File::Refresh(const nlohmann::json& data)
{
    Item::Refresh(data);

    uint64_t newSize; try
    {
        data.at("size").get_to(newSize);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    if (newSize == mBackendSize) return;

    mDebug << mName << ":" << __func__ << "... backend changed size!"
        << " old:" << mBackendSize << " new:" << newSize << " size:" << mSize; mDebug.Info();

    mBackendSize = newSize; 
    uint64_t maxDirty { 0 };

    // get new max size = max(server size, dirty byte) and purge extra mPages
    for (PageMap::reverse_iterator it { mPages.rbegin() }; it != mPages.rend(); )
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
    }

    mSize = std::max(mBackendSize, maxDirty);
}

/*****************************************************/
void File::SubDelete()
{
    mDebug << mName << ":" << __func__ << "()"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.DeleteFile(GetID());

    mDeleted = true;
}

/*****************************************************/
void File::SubRename(const std::string& newName, bool overwrite)
{
    mDebug << mName << ":" << __func__ << " (name:" << newName << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.RenameFile(GetID(), newName, overwrite);
}

/*****************************************************/
void File::SubMove(Folder& newParent, bool overwrite)
{
    mDebug << mName << ":" << __func__ << " (parent:" << newParent.GetName() << ")"; mDebug.Info();

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
File::Page& File::GetPage(const uint64_t index, const size_t minsize)
{
    PageMap::iterator it { mPages.find(index) };

    if (it == mPages.end())
    {
        const uint64_t offset { index*mPageSize };
        const size_t readsize { min64st(mSize-offset, mPageSize) };
        
        mDebug << __func__ << "... index:" << index << " offset:" << offset << " readsize:" << readsize; mDebug.Info();

        bool hasData { readsize > 0 && offset < mBackendSize };

        const std::string data(hasData ? mBackend.ReadFile(GetID(), offset, readsize) : "");

        // for the first page we keep it minimal to save memory on small files
        // for subsequent mPages we allocate the full size ahead of time for speed
        const size_t pageSize { (index == 0) ? readsize : mPageSize };

        it = mPages.emplace(index, pageSize).first;

        char* buf { reinterpret_cast<char*>(it->second.data.data()) };

        std::copy(data.cbegin(), data.cend(), buf);
    }

    Page& page(it->second);

    if (page.data.size() < minsize) 
        page.data.resize(minsize);

    return page;
}

/*****************************************************/
void File::FlushCache(bool nothrow)
{
    if (mDeleted) return;

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

            if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const Utilities::Exception& e) { 
                mDebug << __func__ << "... Ignoring Error: " << e.what(); mDebug.Error(); }

            page.dirty = false; mBackendSize = std::max(mBackendSize, pageOffset+pageSize);
        }
    }
}

/*****************************************************/
void File::ReadPage(std::byte* buffer, const uint64_t index, const size_t offset, const size_t length)
{
    if (mDebug) { mDebug << mName << ":" << __func__ << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    const Page& page(GetPage(index));

    const auto iOffset { static_cast<Page::Data::iterator::difference_type>(offset) };
    const auto iLength { static_cast<Page::Data::iterator::difference_type>(length) };

    std::copy(page.data.cbegin()+iOffset, page.data.cbegin()+iOffset+iLength, buffer);
}

/*****************************************************/
void File::WritePage(const std::byte* buffer, const uint64_t index, const size_t offset, const size_t length)
{
    if (mDebug) { mDebug << mName << ":" << __func__ << " (index:" << index << " offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    Page& page(GetPage(index, offset+length)); page.dirty = true;

    const auto iOffset { static_cast<Page::Data::iterator::difference_type>(offset) };
    const auto iLength { static_cast<Page::Data::iterator::difference_type>(length) };

    std::copy(buffer, buffer+iLength, page.data.begin()+iOffset);
}

/*****************************************************/
size_t File::ReadBytes(std::byte* buffer, const uint64_t offset, size_t length)
{    
    if (mDebug) { mDebug << mName << ":" << __func__ << " (offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    if (offset >= mSize) return 0;

    length = min64st(mSize-offset, length);

    if (mBackend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        const std::string data(mBackend.ReadFile(GetID(), offset, length));

        std::copy(data.cbegin(), data.cend(), reinterpret_cast<char*>(buffer));
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        const uint64_t index { byte / mPageSize };
        const size_t pOffset { static_cast<size_t>(byte - index*mPageSize) };
        const size_t pLength { min64st(length+offset-byte, mPageSize-pOffset) };

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
    if (mDebug) { mDebug << mName << ":" << __func__ << " (offset:" << offset << " length:" << length << ")"; mDebug.Info(); }

    if (isReadOnly()) throw ReadOnlyException();

    const FSConfig::WriteMode writeMode(GetWriteMode());
    if (writeMode == FSConfig::WriteMode::NONE) throw WriteTypeException();

    if (mBackend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
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
    }
}

/*****************************************************/
void File::Truncate(const uint64_t newSize)
{    
    mDebug << mName << ":" << __func__ << " (size:" << newSize << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    if (GetWriteMode() < FSConfig::WriteMode::RANDOM) throw WriteTypeException();

    mBackend.TruncateFile(GetID(), newSize); 

    mSize = newSize; mBackendSize = newSize;

    for (PageMap::iterator it { mPages.begin() }; it != mPages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/mPageSize)
        {
            it = mPages.erase(it); // remove past end
        }
        else ++it; // move to next page
    }
}

} // namespace FSItems
} // namespace Andromeda
