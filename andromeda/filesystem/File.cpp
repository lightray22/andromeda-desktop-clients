#include <nlohmann/json.hpp>

#include "File.hpp"
#include "Backend.hpp"
#include "Folder.hpp"

/*****************************************************/
File::File(Backend& backend, Folder& parent, const nlohmann::json& data) : 
    Item(backend, parent, data), debug("File",this)
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
void File::Delete()
{
    backend.DeleteFile(this->id);

    GetParent().RemoveItem(this->name);
}