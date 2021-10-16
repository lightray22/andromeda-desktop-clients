#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Backend.hpp"
#include "Folder.hpp"
#include "FSConfig.hpp"

/*****************************************************/
File::File(Backend& backend, Folder& parent, const nlohmann::json& data) : 
    Item(backend, data), debug("File",this)
{
    debug << this->name << ":" << __func__ << "()"; debug.Info();

    this->parent = &parent;

    std::string fsid; try
    {
        data.at("size").get_to(this->size);
        this->backendSize = this->size;

        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    this->fsConfig = &FSConfig::LoadByID(backend, fsid);

    const size_t fsChunk = this->fsConfig->GetChunkSize();
    const size_t cfChunk = backend.GetConfig().GetOptions().pageSize;

    auto ceil = [](int x, int y) { return (x + y - 1) / y; };
    this->pageSize = fsChunk ? ceil(cfChunk,fsChunk)*fsChunk : cfChunk;

    debug << this->name << ":" << __func__ << "... fsChunk:" << fsChunk << " cfChunk:" << cfChunk << " pageSize:" << pageSize; debug.Info();
}

/*****************************************************/
void File::Refresh(const nlohmann::json& data)
{
    Item::Refresh(data);

    size_t newSize; try
    {
        data.at("size").get_to(newSize);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    if (newSize == this->backendSize) return;

    debug << this->name << ":" << __func__ << "()... backend changed size!"
        << " old:" << this->backendSize << " new:" << newSize << " size:" << this->size; debug.Info();

    this->backendSize = newSize; size_t maxDirty = 0;

    // get new max size = max(server size, dirty byte) and purge extra pages
    for (PageMap::reverse_iterator it = pages.rbegin(); it != pages.rend(); )
    {
        size_t pageStart = it->first * pageSize; Page& page(it->second);

        if (pageStart >= this->backendSize)
        {
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

    backend.DeleteFile(this->id);

    this->deleted = true;
}

/*****************************************************/
void File::SubRename(const std::string& name, bool overwrite)
{
    debug << this->name << ":" << __func__ << "(name:" << name << ")"; debug.Info();

    backend.RenameFile(this->id, name, overwrite);
}

/*****************************************************/
void File::SubMove(Folder& parent, bool overwrite)
{
    debug << this->name << ":" << __func__ << "(parent:" << parent.GetName() << ")"; debug.Info();

    backend.MoveFile(this->id, parent.GetID(), overwrite);
}

/*****************************************************/
File::Page& File::GetPage(const size_t index, const size_t minsize)
{
    PageMap::iterator it = pages.find(index);

    if (it == pages.end())
    {
        const size_t offset = index*this->pageSize;
        const size_t rsize = std::min(this->size-offset, this->pageSize);
        
        debug << __func__ << "()... index:" << index << " offset:" << offset << " rsize:" << rsize; debug.Info();

        bool hasData = rsize > 0 && offset < this->backendSize;

        const std::string data(hasData ? backend.ReadFile(this->id, offset, rsize) : "");

        // for the first page we keep it minimal to save memory on small files
        // for subsequent pages we allocate the full size ahead of time for speed
        const size_t pageSize = (index == 0) ? rsize : this->pageSize;

        it = pages.emplace(index, pageSize).first;

        char* buf = reinterpret_cast<char*>(it->second.data.data());

        std::copy(data.cbegin(), data.cend(), buf);
    }

    Page& page(it->second);

    if (page.data.size() < minsize) 
        page.data.resize(minsize);

    return page;
}

/*****************************************************/
void File::FlushCache()
{
    if (this->deleted) return;

    debug << this->name << ":" << __func__ << "()"; debug.Info();

    for (PageMap::iterator it = pages.begin(); it != pages.end(); it++)
    {
        const size_t index(it->first); Page& page(it->second);

        if (page.dirty)
        {
            const size_t offset = index*this->pageSize;
            const size_t size = std::min(this->size-offset, this->pageSize);

            std::string data(reinterpret_cast<const char*>(page.data.data()), size);

            if (debug) { debug << __func__ << "()... index:" << index << " offset:" << offset << " size:" << size; debug.Info(); }

            backend.WriteFile(this->id, offset, data); page.dirty = false;
            
            this->backendSize = std::max(this->backendSize, offset+size);
        }
    }
}

/*****************************************************/
void File::ReadPage(std::byte* buffer, const size_t index, const size_t offset, const size_t length)
{
    if (debug) { debug << this->name << ":" << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; debug.Info(); }

    const Page& page(GetPage(index));

    std::copy(page.data.cbegin()+offset, page.data.cbegin()+offset+length, buffer);
}

/*****************************************************/
void File::WritePage(const std::byte* buffer, const size_t index, const size_t offset, const size_t length)
{
    if (debug) { debug << this->name << ":" << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; debug.Info(); }

    Page& page(GetPage(index, offset+length)); page.dirty = true;

    std::copy(buffer, buffer+length, page.data.begin()+offset);
}

/*****************************************************/
size_t File::ReadBytes(std::byte* buffer, const size_t offset, size_t length)
{    
    if (debug) { debug << this->name << ":" << __func__ << "(offset:" << offset << " length:" << length << ")"; debug.Info(); }

    if (offset >= this->size) return 0;

    length = std::min(this->size - offset, length);

    if (backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        const std::string data(backend.ReadFile(this->id, offset, length));

        std::copy(data.cbegin(), data.cend(), reinterpret_cast<char*>(buffer));
    }
    else for (size_t byte = offset; byte < offset+length; )
    {
        size_t index = byte / pageSize;
        size_t pOffset = byte - index*pageSize;

        size_t pLength = std::min(length+offset-byte, pageSize-pOffset);

        if (debug) { debug << __func__ << "()... size:" << this->size << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; debug.Info(); }

        ReadPage(buffer, index, pOffset, pLength);

        buffer += pLength; byte += pLength;
    }

    return length;
}

/*****************************************************/
void File::WriteBytes(const std::byte* buffer, const size_t offset, const size_t length)
{
    if (debug) { debug << this->name << ":" << __func__ << "offset:" << offset << " length:" << length << ")"; debug.Info(); }

    if (backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        const std::string data(reinterpret_cast<const char*>(buffer), length);

        backend.WriteFile(this->id, offset, data);

        this->size = std::max(this->size, offset+length);
    }
    else for (size_t byte = offset; byte < offset+length; )
    {
        size_t index = byte / pageSize;
        size_t pOffset = byte - index*pageSize;

        size_t pLength = std::min(length+offset-byte, pageSize-pOffset);

        if (debug) { debug << __func__ << "()... size:" << size << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; debug.Info(); }

        WritePage(buffer, index, pOffset, pLength);   
    
        this->size = std::max(this->size, byte+pLength);
        
        buffer += pLength; byte += pLength;
    }
}

/*****************************************************/
void File::Truncate(const size_t size)
{    
    debug << this->name << ":" << __func__ << "(size:" << size << ")"; debug.Info();

    this->backend.TruncateFile(this->id, size); 

    this->size = size; this->backendSize = size;

    for (PageMap::iterator it = pages.begin(); it != pages.end(); )
    {
        if (!size || it->first > (size-1)/pageSize)
        {
            it = pages.erase(it); // remove past end
        }
        else it++; // move to next page
    }
}
