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
