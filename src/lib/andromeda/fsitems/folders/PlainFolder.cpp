
#include <nlohmann/json.hpp>

#include "PlainFolder.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/FSConfig.hpp"
#include "andromeda/fsitems/File.hpp"

namespace Andromeda {
namespace FSItems {
namespace Folders {

/*****************************************************/
std::unique_ptr<PlainFolder> PlainFolder::LoadByID(Backend& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<PlainFolder>(backend, &data, nullptr, true);
}

/*****************************************************/
PlainFolder::PlainFolder(Backend& backend, const nlohmann::json* data, Folder* parent, bool haveItems) : 
    Folder(backend), debug("PlainFolder",this)
{
    debug << __func__ << "()"; debug.Info();

    this->parent = parent;
    
    if (data != nullptr)
    {
        Initialize(*data);

        std::string fsid; try
        {
            if (haveItems) Folder::LoadItemsFrom(*data);

            data->at("filesystem").get_to(fsid);
        }
        catch (const nlohmann::json::exception& ex) {
            throw Backend::JSONErrorException(ex.what()); }

        this->fsConfig = &FSConfig::LoadByID(backend, fsid);
    }
}

/*****************************************************/
void PlainFolder::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    Folder::LoadItemsFrom(backend.GetFolder(GetID()));
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(backend.CreateFile(GetID(), name));

    std::unique_ptr<File> file(std::make_unique<File>(backend, data, *this));

    this->itemMap[file->GetName()] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(backend.CreateFolder(GetID(), name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(backend, &data, this));

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

    if (isReadOnly()) throw ReadOnlyException();

    backend.DeleteFolder(GetID());
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& name, bool overwrite)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    backend.RenameFolder(GetID(), name, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(Folder& parent, bool overwrite)
{
    debug << __func__ << "(parent:" << parent.GetName() << ")"; debug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    backend.MoveFolder(GetID(), parent.GetID(), overwrite);
}

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders
