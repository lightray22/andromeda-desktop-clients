#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "Backend.hpp"

/*****************************************************/
Folder::Folder(Backend& backend, const std::string& id) :
    BaseFolder(backend), debug("Folder",this), id(id)
{
    debug << __func__ << "(id:" << id << ")"; debug.Out();

    nlohmann::json folder(backend.GetFolder(id));

    try
    {
        this->id = folder["id"]; 
        this->name = folder["name"];
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
};