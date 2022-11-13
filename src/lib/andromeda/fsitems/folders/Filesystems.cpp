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
    Folder(backend), mDebug("Filesystems",this)
{
    mDebug << __func__ << "()"; mDebug.Info();

    backend.RequireAuthentication();

    mName = "Filesystems";
    mParent = &parent;
}

/*****************************************************/
void Filesystems::LoadItems()
{
    mDebug << __func__ << "()"; mDebug.Info();

    nlohmann::json data(mBackend.GetFilesystems());

    Folder::NewItemMap newItems;

    NewItemFunc newFilesystem { [&](const nlohmann::json& fsJ)->std::unique_ptr<Item> {
        return std::make_unique<Filesystem>(mBackend, fsJ, this); } };

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
void Filesystems::SubRenameItem(Item& item, const std::string& newName, bool overwrite)
{
    item.Rename(newName, overwrite, true);
}

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders
