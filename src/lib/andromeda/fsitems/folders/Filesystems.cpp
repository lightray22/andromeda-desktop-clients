#include <functional>
#include <nlohmann/json.hpp>

#include "Filesystems.hpp"
#include "Filesystem.hpp"
#include "andromeda/Backend.hpp"

namespace Andromeda {
namespace FSItems {
namespace Folders {

/*****************************************************/
Filesystems::Filesystems(Backend& backend, Folder& parent) : 
    Folder(backend), debug("Filesystems",this)
{
    debug << __func__ << "()"; debug.Info();

    backend.RequireAuthentication();

    this->name = "Filesystems";
    this->parent = &parent;
}

/*****************************************************/
void Filesystems::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    nlohmann::json data(backend.GetFilesystems());

    Folder::NewItemMap newItems;

    NewItemFunc newFilesystem = [&](const nlohmann::json& fsJ)->std::unique_ptr<Item> {
        return std::make_unique<Filesystem>(backend, fsJ, this); };

    try
    {
        for (const nlohmann::json& fsJ : data)
        {
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(fsJ.at("name")),
                std::forward_as_tuple(fsJ, newFilesystem));
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    SyncContents(newItems);
}

/*****************************************************/
void Filesystems::SubDeleteItem(Item& item)
{
    item.Delete(true);
}

/*****************************************************/
void Filesystems::SubRenameItem(Item& item, const std::string& name, bool overwrite)
{
    item.Rename(name, overwrite, true);
}

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders
