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
void File::Delete(bool internal)
{
    if (internal) 
    {
        debug << __func__ << "()"; debug.Info();

        backend.DeleteFile(this->id);
    }
    else this->parent.DeleteItem(this->name);
}

/*****************************************************/
void File::Rename(const std::string& name, bool overwrite, bool internal)
{
    if (internal)
    {
        debug << __func__ << "(name:" << name << ")"; debug.Info();

        backend.RenameFile(this->id, name, overwrite); this->name = name;
    }
    else this->parent.RenameItem(this->name, name, overwrite);
}
