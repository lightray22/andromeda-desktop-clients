
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Folder.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/FSConfig.hpp"

namespace Andromeda {
namespace FSItems {

/*****************************************************/
Item::Item(Backend& backend) : 
    backend(backend), debug("Item",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
void Item::Initialize(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();

    try
    {
        data.at("id").get_to(this->id);
        data.at("name").get_to(this->name);

        if (data.contains("dates"))
        {
            data.at("dates").at("created").get_to(this->created);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    Refresh(data);
}

/*****************************************************/
void Item::Refresh(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();

    try
    {
        data.at("name").get_to(this->name);

        debug << __func__ << "... name:" << this->name; debug.Info();

        const nlohmann::json& modifiedJ(data.at("dates").at("modified"));
        if (!modifiedJ.is_null()) modifiedJ.get_to(this->modified);

        const nlohmann::json& accessedJ(data.at("dates").at("accessed"));
        if (!accessedJ.is_null()) accessedJ.get_to(this->accessed);
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
const FSConfig& Item::GetFSConfig() const
{
    if (this->fsConfig == nullptr)
        throw NullFSConfigException();

    return *this->fsConfig;
}

/*****************************************************/
bool Item::isReadOnly() const
{
    const Config& config(backend.GetConfig());

    bool retval { config.isReadOnly() || config.GetOptions().readOnly };

    if (HasFSConfig()) retval |= GetFSConfig().isReadOnly();

    return retval;
}

/*****************************************************/
void Item::Delete(bool internal)
{
    if (internal || !HasParent()) SubDelete();
    else GetParent().DeleteItem(this->name);
}

/*****************************************************/
void Item::Rename(const std::string& newName, bool overwrite, bool internal)
{
    if (internal || !HasParent())
    {
        SubRename(newName, overwrite); 
        this->name = newName;
    }
    else GetParent().RenameItem(this->name, newName, overwrite);
}

/*****************************************************/
void Item::Move(Folder& newParent, bool overwrite, bool internal)
{
    if (internal)
    {
        SubMove(newParent, overwrite); 
        this->parent = &newParent;
    }
    else GetParent().MoveItem(this->name, newParent, overwrite);
}

} // namespace FSItems
} // namespace Andromeda
