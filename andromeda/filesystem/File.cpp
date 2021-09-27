#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Backend.hpp"
#include "Folder.hpp"

/*****************************************************/
File::File(Backend& backend, Folder& parent, const nlohmann::json& data) : 
    Item(backend, data), parent(parent), debug("File",this)
{
    debug << __func__ << "()"; debug.Info();
    
    try
    {
        data.at("size").get_to(this->size);
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
