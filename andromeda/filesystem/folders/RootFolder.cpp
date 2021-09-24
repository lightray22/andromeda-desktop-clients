#include <nlohmann/json.hpp>

#include "RootFolder.hpp"
#include "Backend.hpp"

/*****************************************************/
std::unique_ptr<RootFolder> RootFolder::LoadByID(Backend& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<RootFolder>(backend, data);
}

/*****************************************************/
RootFolder::RootFolder(Backend& backend) : 
    Folder(backend), debug("RootFolder",this)
{
    debug << __func__ << "()"; debug.Info();    
}

/*****************************************************/
RootFolder::RootFolder(Backend& backend, const nlohmann::json& data) :
    Folder(backend, data, true), debug("RootFolder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
void RootFolder::Delete()
{
    throw DeleteRootException();
}