
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Folder.hpp"
#include "Backend.hpp"
#include "FSConfig.hpp"

/*****************************************************/
Item::Item(Backend& backend, const nlohmann::json* data) : 
    backend(backend), debug("Item",this)
{
    debug << __func__ << "()"; debug.Info();

    if (data != nullptr) Initialize(*data);
}

/*****************************************************/
void Item::Initialize(const nlohmann::json& data)
{
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
    try
    {
        data.at("name").get_to(this->name);

        debug << __func__ << "... name:" << this->name; debug.Info();

        const nlohmann::json& modified(data.at("dates").at("modified"));
        if (!modified.is_null()) modified.get_to(this->modified);

        const nlohmann::json& accessed(data.at("dates").at("accessed"));
        if (!accessed.is_null()) accessed.get_to(this->accessed);
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

    bool retval = config.isReadOnly() || config.GetOptions().readOnly;

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
