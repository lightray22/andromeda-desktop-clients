#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Folder.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/FSConfig.hpp"

namespace Andromeda {
namespace FSItems {

/*****************************************************/
File::File(Backend& backend, const nlohmann::json& data, Folder& parent) : 
    Item(backend), debug("File",this)
{
    debug << this->name << ":" << __func__ << "()"; debug.Info();

    Initialize(data); this->parent = &parent;

    std::string fsid; try
    {
        data.at("size").get_to(this->size);
        this->backendSize = this->size;

        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    this->fsConfig = &FSConfig::LoadByID(backend, fsid);

    const uint64_t fsChunk { this->fsConfig->GetChunkSize() };
    const uint64_t cfChunk { backend.GetConfig().GetOptions().pageSize };

    auto ceil { [](auto x, auto y) { return (x + y - 1) / y; } };
    this->pageSize = fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk;

    debug << this->name << ":" << __func__ << "... fsChunk:" << fsChunk << " cfChunk:" << cfChunk << " pageSize:" << pageSize; debug.Info();
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

    if (newSize == this->backendSize) return;

    debug << this->name << ":" << __func__ << "()... backend changed size!"
        << " old:" << this->backendSize << " new:" << newSize << " size:" << this->size; debug.Info();

    this->backendSize = newSize; 
    uint64_t maxDirty { 0 };

    // get new max size = max(server size, dirty byte) and purge extra pages
    for (PageMap::reverse_iterator it { pages.rbegin() }; it != pages.rend(); )
    {
        uint64_t pageStart { it->first * pageSize };

        if (pageStart >= this->backendSize)
        {
            Page& page(it->second);

            // dirty pages will extend the file again when written
            if (page.dirty) { maxDirty = std::min(this->size, pageStart + pageSize); break; }

            // hacky reverse_iterator version of it = pages.erase(it);
            else it = decltype(it)(pages.erase(std::next(it).base())); // erase deleted page
        }
        else break; // future pageStarts are smaller
    }

    this->size = std::max(this->backendSize, maxDirty);
}

/*****************************************************/
void File::SubDelete()
{
    debug << this->name << ":" << __func__ << "()"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    backend.DeleteFile(GetID());

    this->deleted = true;
}

/*****************************************************/
void File::SubRename(const std::string& newName, bool overwrite)
{
    debug << this->name << ":" << __func__ << " (name:" << newName << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    backend.RenameFile(GetID(), newName, overwrite);
}

/*****************************************************/
void File::SubMove(Folder& newParent, bool overwrite)
{
    debug << this->name << ":" << __func__ << " (parent:" << newParent.GetName() << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    backend.MoveFile(GetID(), newParent.GetID(), overwrite);
}

/*****************************************************/
FSConfig::WriteMode File::GetWriteMode() const
{
    FSConfig::WriteMode writeMode(GetFSConfig().GetWriteMode());

    if (writeMode >= FSConfig::WriteMode::RANDOM && 
        !backend.GetConfig().canRandWrite())
    {
        writeMode = FSConfig::WriteMode::APPEND;
    }

    return writeMode;
}

/*****************************************************/
File::Page& File::GetPage(const uint64_t index, const uint64_t minsize)
{
    PageMap::iterator it { pages.find(index) };

    if (it == pages.end())
    {
        const uint64_t offset { index*this->pageSize };
        const uint64_t readsize { std::min(this->size-offset, this->pageSize) };
        
        debug << __func__ << "()... index:" << index << " offset:" << offset << " readsize:" << readsize; debug.Info();

        bool hasData { readsize > 0 && offset < this->backendSize };

        const std::string data(hasData ? backend.ReadFile(GetID(), offset, readsize) : "");

        // for the first page we keep it minimal to save memory on small files
        // for subsequent pages we allocate the full size ahead of time for speed
        const uint64_t pageSizeI { (index == 0) ? readsize : this->pageSize };

        it = pages.emplace(index, pageSizeI).first;

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
    if (this->deleted) return;

    debug << this->name << ":" << __func__ << "()"; debug.Info();

    for (PageMap::iterator it { pages.begin() }; it != pages.end(); ++it)
    {
        const uint64_t index(it->first); Page& page(it->second);

        if (page.dirty)
        {
            const uint64_t pageOffset { index*this->pageSize };
            const uint64_t pageSizeI { std::min(this->size-pageOffset, this->pageSize) };

            std::string data(reinterpret_cast<const char*>(page.data.data()), pageSizeI);

            if (debug) { debug << __func__ << "()... index:" << index << " offset:" << pageOffset << " size:" << pageSizeI; debug.Info(); }

            auto writeFunc { [&]()->void { backend.WriteFile(GetID(), pageOffset, data); } };

            if (!nothrow) writeFunc(); else try { writeFunc(); } catch (const Utilities::Exception& e) { 
                debug << __func__ << "()... Ignoring Error: " << e.what(); debug.Error(); }

            page.dirty = false; this->backendSize = std::max(this->backendSize, pageOffset+pageSizeI);
        }
    }
}

/*****************************************************/
void File::ReadPage(std::byte* buffer, const uint64_t index, const uint64_t offset, const size_t length)
{
    if (debug) { debug << this->name << ":" << __func__ << " (index:" << index << " offset:" << offset << " length:" << length << ")"; debug.Info(); }

    const Page& page(GetPage(index));

    std::copy(page.data.cbegin()+offset, page.data.cbegin()+offset+length, buffer);
}

/*****************************************************/
void File::WritePage(const std::byte* buffer, const uint64_t index, const uint64_t offset, const size_t length)
{
    if (debug) { debug << this->name << ":" << __func__ << " (index:" << index << " offset:" << offset << " length:" << length << ")"; debug.Info(); }

    Page& page(GetPage(index, offset+length)); page.dirty = true;

    std::copy(buffer, buffer+length, page.data.begin()+offset);
}

/*****************************************************/
size_t File::ReadBytes(std::byte* buffer, const uint64_t offset, size_t length)
{    
    if (debug) { debug << this->name << ":" << __func__ << " (offset:" << offset << " length:" << length << ")"; debug.Info(); }

    if (offset >= this->size) return 0;

    length = std::min(this->size - offset, length);

    if (backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        const std::string data(backend.ReadFile(GetID(), offset, length));

        std::copy(data.cbegin(), data.cend(), reinterpret_cast<char*>(buffer));
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        uint64_t index { byte / pageSize };
        uint64_t pOffset { byte - index*pageSize };

        uint64_t pLength { std::min(length+offset-byte, pageSize-pOffset) };

        if (debug) { debug << __func__ << "()... size:" << this->size << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; debug.Info(); }

        ReadPage(buffer, index, pOffset, pLength);

        buffer += pLength; byte += pLength;
    }

    return length;
}

/*****************************************************/
void File::WriteBytes(const std::byte* buffer, const uint64_t offset, const size_t length)
{
    if (debug) { debug << this->name << ":" << __func__ << " (offset:" << offset << " length:" << length << ")"; debug.Info(); }

    if (isReadOnly()) throw ReadOnlyException();

    const FSConfig::WriteMode writeMode(GetWriteMode());
    if (writeMode == FSConfig::WriteMode::NONE) throw WriteTypeException();

    if (backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        if (writeMode == FSConfig::WriteMode::APPEND 
            && offset != this->backendSize) throw WriteTypeException();

        const std::string data(reinterpret_cast<const char*>(buffer), length);

        backend.WriteFile(GetID(), offset, data);

        this->size = std::max(this->size, offset+length); 
        this->backendSize = this->size;
    }
    else for (uint64_t byte { offset }; byte < offset+length; )
    {
        if (writeMode == FSConfig::WriteMode::APPEND)
        {
            // allowed if (==fileSize and startOfPage) OR (within dirty page)
            if (!(offset == this->size && offset % pageSize == 0) && 
                !(GetPage(offset/pageSize).dirty)) throw WriteTypeException();
        }

        uint64_t index { byte / pageSize };
        uint64_t pOffset { byte - index*pageSize };

        uint64_t pLength { std::min(length+offset-byte, pageSize-pOffset) };

        if (debug) { debug << __func__ << "()... size:" << size << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; debug.Info(); }

        WritePage(buffer, index, pOffset, pLength);   
    
        this->size = std::max(this->size, byte+pLength);
        
        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
void File::Truncate(const uint64_t newSize)
{    
    debug << this->name << ":" << __func__ << " (size:" << newSize << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    if (GetWriteMode() < FSConfig::WriteMode::RANDOM) throw WriteTypeException();

    this->backend.TruncateFile(GetID(), newSize); 

    this->size = newSize; this->backendSize = newSize;

    for (PageMap::iterator it { pages.begin() }; it != pages.end(); )
    {
        if (!newSize || it->first > (newSize-1)/pageSize)
        {
            it = pages.erase(it); // remove past end
        }
        else ++it; // move to next page
    }
}

} // namespace FSItems
} // namespace Andromeda
