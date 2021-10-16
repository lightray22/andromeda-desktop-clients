
#include <nlohmann/json.hpp>

#include "PlainFolder.hpp"
#include "Backend.hpp"
#include "../File.hpp"

/*****************************************************/
std::unique_ptr<PlainFolder> PlainFolder::LoadByID(Backend& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<PlainFolder>(backend, data);
}

/*****************************************************/
PlainFolder::PlainFolder(Backend& backend) : 
    Folder(backend), debug("PlainFolder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
PlainFolder::PlainFolder(Backend& backend, const nlohmann::json& data, bool haveItems) : 
    Folder(backend, data), debug("PlainFolder",this)
{
    debug << __func__ << "()"; debug.Info();
    
    try
    {
        if (haveItems) Folder::LoadItemsFrom(data);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
PlainFolder::PlainFolder(Backend& backend, Folder& parent, const nlohmann::json& data, bool haveItems) : 
    PlainFolder(backend, data, haveItems) { this->parent = &parent; }

/*****************************************************/
void PlainFolder::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    Folder::LoadItemsFrom(backend.GetFolder(this->id));
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    nlohmann::json data(backend.CreateFile(this->id, name));

    std::unique_ptr<File> file(std::make_unique<File>(backend, *this, data));

    this->itemMap[file->GetName()] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    nlohmann::json data(backend.CreateFolder(this->id, name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(backend, *this, data));

    this->itemMap[folder->GetName()] = std::move(folder);
}

/*****************************************************/
void PlainFolder::SubDeleteItem(Item& item)
{
    item.Delete(true);
}

/*****************************************************/
void PlainFolder::SubRenameItem(Item& item, const std::string& name, bool overwrite)
{
    item.Rename(name, overwrite, true);
}

/*****************************************************/
void PlainFolder::SubMoveItem(Item& item, Folder& parent, bool overwrite)
{
    item.Move(parent, overwrite, true);
}

/*****************************************************/
void PlainFolder::SubDelete()
{
    debug << __func__ << "()"; debug.Info();

    backend.DeleteFolder(this->id);
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& name, bool overwrite)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    backend.RenameFolder(this->id, name, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(Folder& parent, bool overwrite)
{
    debug << __func__ << "(parent:" << parent.GetName() << ")"; debug.Info();

    backend.MoveFolder(this->id, parent.GetID(), overwrite);
}
