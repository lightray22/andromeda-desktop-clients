#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Backend.hpp"
#include "Folder.hpp"

/*****************************************************/
File::File(Backend& backend, Folder& parent, const nlohmann::json& data) : 
    Item(backend, data), debug("File",this)
{
    debug << __func__ << "()"; debug.Info();

    this->parent = &parent;
    
    try
    {
        data.at("size").get_to(this->size);

        this->backendSize = this->size;
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void File::SubDelete()
{
    debug << __func__ << "()"; debug.Info();

    backend.DeleteFile(this->id);
}

/*****************************************************/
void File::SubRename(const std::string& name, bool overwrite)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    backend.RenameFile(this->id, name, overwrite);
}

/*****************************************************/
void File::SubMove(Folder& parent, bool overwrite)
{
    debug << __func__ << "(parent:" << parent.GetName() << ")"; debug.Info();

    backend.MoveFile(this->id, parent.GetID(), overwrite);
}

/*****************************************************/
File::Page& File::GetPage(const size_t index)
{
    PageMap::iterator it = pages.find(index);

    if (it == pages.end())
    {
        const size_t offset = index*this->pageSize;
        const size_t size = std::min(this->size-offset, this->pageSize);
        
        debug << __func__ << "()... index:" << index << " offset:" << offset << " size:" << size; debug.Details();

        bool hasData = size > 0 && offset < this->backendSize;

        const std::string data(hasData ? backend.ReadFile(this->id, offset, size) : "");

        it = pages.emplace(index, this->pageSize).first;

        Page& page(it->second); char* buf = reinterpret_cast<char*>(page.data.data());

        std::copy(data.cbegin(), data.cend(), buf);
    }

    return it->second;
}

/*****************************************************/
void File::ReadPage(std::byte* buffer, const size_t index, const size_t offset, const size_t length)
{
    debug << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; debug.Info();

    const Page& page(GetPage(index));

    std::copy(page.data.cbegin()+offset, page.data.cbegin()+offset+length, buffer);
}

/*****************************************************/
void File::WritePage(const std::byte* buffer, const size_t index, const size_t offset, const size_t length)
{
    debug << __func__ << "(index:" << index << " offset:" << offset << " length:" << length << ")"; debug.Info();

    Page& page(GetPage(index)); page.dirty = true;

    std::copy(buffer, buffer+length, page.data.begin()+offset);
}

/*****************************************************/
size_t File::ReadBytes(std::byte* buffer, const size_t offset, size_t length)
{    
    debug << __func__ << "(name:" << name << " offset:" << offset << " length:" << length << ")"; debug.Info();

    if (offset >= this->size) return 0;

    length = std::min(this->size - offset, length);

    if (backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        const std::string data(backend.ReadFile(this->id, offset, this->size));

        std::copy(data.cbegin(), data.cend(), reinterpret_cast<char*>(buffer));
    }
    else for (size_t byte = offset; byte < offset+length; )
    {
        size_t index = byte / pageSize;
        size_t pOffset = byte - index*pageSize;

        size_t pLength = std::min(length+offset-byte, pageSize-pOffset);

        debug << __func__ << "()... size:" << this->size << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; debug.Details();

        ReadPage(buffer, index, pOffset, pLength);

        buffer += pLength; byte += pLength;
    }

    return length;
}

/*****************************************************/
void File::WriteBytes(const std::byte* buffer, const size_t offset, const size_t length)
{
    debug << __func__ << "(name:" << name << " offset:" << offset << " length:" << length << ")"; debug.Info();

    if (backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE)
    {
        const std::string data(reinterpret_cast<const char*>(buffer), length);

        backend.WriteFile(this->id, offset, data);
    }
    else for (size_t byte = offset; byte < offset+length; )
    {
        size_t index = byte / pageSize;
        size_t pOffset = byte - index*pageSize;

        size_t pLength = std::min(length+offset-byte, pageSize-pOffset);

        debug << __func__ << "()... size:" << size << " byte:" << byte << " index:" << index 
            << " pOffset:" << pOffset << " pLength:" << pLength; debug.Details();

        WritePage(buffer, index, pOffset, pLength);   
    
        buffer += pLength; byte += pLength;
    }

    this->size = std::max(this->size, offset+length);
}

/*****************************************************/
void File::Truncate(const size_t size)
{    
    debug << __func__ << "(name:" << name << " size:" << size << ")"; debug.Info();

    this->backend.TruncateFile(this->id, size); 
    
    this->size = size; 
    this->backendSize = size;

    for (PageMap::iterator it = pages.begin(); it != pages.end(); )
    {
        if (it->first > size/pageSize)
        {
            it = pages.erase(it);
        }
        else it++;
    }
}

/*****************************************************/
void File::FlushCache()
{
    debug << __func__ << "()"; debug.Info();

    for (PageMap::iterator it = pages.begin(); it != pages.end(); it++)
    {
        const size_t index(it->first); 
        Page& page(it->second);

        if (page.dirty)
        {
            const size_t offset = index*this->pageSize;
            const size_t size = std::min(this->size-offset, this->pageSize);

            std::string data(reinterpret_cast<const char*>(page.data.data()), size);

            debug << __func__ << "()... index:" << index << " offset:" << offset << " size:" << size; debug.Details();

            backend.WriteFile(this->id, offset, data); page.dirty = false;
        }
    }

    this->backendSize = size;
}
