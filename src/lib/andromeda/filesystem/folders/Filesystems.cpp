#include <functional>
#include <nlohmann/json.hpp>

#include "Filesystems.hpp"
#include "Filesystem.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
Filesystems::Filesystems(BackendImpl& backend, Folder& parent) : 
    Folder(backend), mDebug("Filesystems",this)
{
    MDBG_INFO("()");

    backend.RequireAuthentication();

    mName = "Filesystems";
    mParent = &parent;
}

/*****************************************************/
void Filesystems::LoadItems()
{
    MDBG_INFO("()");

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
        throw BackendImpl::JSONErrorException(ex.what()); }

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
} // namespace Filesystem
} // namespace Folders
