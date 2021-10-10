
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

        const nlohmann::json& modified(data.at("dates").at("modified"));
        if (!modified.is_null()) modified.get_to(this->modified);

        const nlohman::json& accessed(data.at("dates").at("accessed"));
        if (!accessed.if_null()) accessed.get_to(this->accessed);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
Folder& Item::GetParent() const     
{ 
    if (this->parent == nullptr) 
        throw NullParentException();
            
    return *this->parent; 
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
        SubRename(name, overwrite); 
        this->name = name;
    }
    else GetParent().RenameItem(this->name, name, overwrite);
}

/*****************************************************/
void Item::Move(Folder& parent, bool overwrite, bool internal)
{
    if (internal)
    {
        SubMove(parent, overwrite); 
        this->parent = &parent;
    }
    else GetParent().MoveItem(this->name, parent, overwrite);
}
