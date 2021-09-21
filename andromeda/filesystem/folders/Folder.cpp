
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "Backend.hpp"

/*****************************************************/
Folder::Folder(Backend& backend) : 
    BaseFolder(backend), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
};

/*****************************************************/
Folder::Folder(Backend& backend, const std::string& id) : Folder(backend)
{
    debug << __func__ << "(id:" << id << ")"; debug.Info();

    backend.RequireAuthentication();

    Initialize(backend.GetFolder(id));
};

/*****************************************************/
void Folder::Initialize(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();
    
    try
    {
        data.at("id").get_to(this->id);

        data.at("counters").at("size").get_to(this->size);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    BaseFolder::Initialize(data);
}
