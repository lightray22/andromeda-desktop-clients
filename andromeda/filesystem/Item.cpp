
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Folder.hpp"
#include "Backend.hpp"

/*****************************************************/
Item::Item(Backend& backend) : 
    backend(backend), debug("Item",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Item::Item(Backend& backend, const nlohmann::json& data) : 
    Item(backend)
{
    try
    {
        data.at("id").get_to(this->id);
        data.at("name").get_to(this->name);

        debug << __func__ << "... name:" << this->name; debug.Details();

        data.at("dates").at("created").get_to(this->created);

        if (data.at("dates").contains("modified"))
            data.at("dates").at("modified").get_to(this->modified);

        if (data.at("dates").contains("accessed"))
            data.at("dates").at("accessed").get_to(this->accessed);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Item::Delete(bool internal)
{
    if (internal || !HasParent()) SubDelete();
    else GetParent().DeleteItem(this->name);
}

/*****************************************************/
void Item::Rename(const std::string& name, bool overwrite, bool internal)
{
    if (internal || !HasParent())
    { 
        SubRename(name, overwrite); this->name = name;
    }
    else GetParent().RenameItem(this->name, name, overwrite);
}
