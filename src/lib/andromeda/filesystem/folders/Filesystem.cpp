#include <nlohmann/json.hpp>

#include "Filesystem.hpp"
#include "FSConfig.hpp"
#include "Backend.hpp"

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadByID(Backend& backend, const std::string& fsid)
{
    backend.RequireAuthentication();

    const nlohmann::json data(backend.GetFilesystem(fsid));

    return std::make_unique<Filesystem>(backend, data);
}

/*****************************************************/
Filesystem::Filesystem(Backend& backend, const nlohmann::json& data, Folder* parent) :
    PlainFolder(backend), debug("Filesystem",this) 
{
    debug << __func__ << "()"; debug.Info();

    Item::Initialize(data); this->parent = parent;
    
    this->fsid = this->id; this->id = "";

    this->fsConfig = &FSConfig::LoadByID(backend, this->fsid);
}

/*****************************************************/
void Filesystem::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    const nlohmann::json data(backend.GetFSRoot(this->fsid));

    try
    {
        data.at("id").get_to(this->id); // late load
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    Folder::LoadItemsFrom(data);
}
