#include <nlohmann/json.hpp>

#include "Filesystem.hpp"
#include "Backend.hpp"

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadByID(Backend& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFilesystem(id));

    return Filesystem::LoadFromData(backend, data);
}

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadFromData(Backend& backend, const nlohmann::json& data)
{
    std::string fsid; try
    {
        data.at("id").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
  
    nlohmann::json rdata(backend.GetFSRoot(fsid));

    return std::make_unique<Filesystem>(backend, fsid, rdata);
}

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadFromData(Backend& backend, Folder& parent, const nlohmann::json& data)
{
    std::unique_ptr<Filesystem> filesystem(Filesystem::LoadFromData(backend, data));

    filesystem->parent = &parent; return filesystem;
}

/*****************************************************/
Filesystem::Filesystem(Backend& backend, std::string fsid, const nlohmann::json& rdata) :
    PlainFolder(backend, rdata), fsid(fsid), debug("Filesystem",this) 
{
    debug << __func__ << "()"; debug.Info();
}
