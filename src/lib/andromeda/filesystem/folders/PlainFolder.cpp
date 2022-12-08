
#include <nlohmann/json.hpp>

#include "PlainFolder.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/FSConfig.hpp"
using Andromeda::Filesystem::FSConfig;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
std::unique_ptr<PlainFolder> PlainFolder::LoadByID(BackendImpl& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<PlainFolder>(backend, &data, nullptr, true);
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, const nlohmann::json* data, Folder* parent, bool haveItems) : 
    Folder(backend), mDebug("PlainFolder",this)
{
    mDebug << __func__ << "()"; mDebug.Info();

    mParent = parent;
    
    if (data != nullptr)
    {
        Initialize(*data);

        std::string fsid; try
        {
            if (haveItems) Folder::LoadItemsFrom(*data);

            data->at("filesystem").get_to(fsid);
        }
        catch (const nlohmann::json::exception& ex) {
            throw BackendImpl::JSONErrorException(ex.what()); }

        mFsConfig = &FSConfig::LoadByID(backend, fsid);
    }
}

/*****************************************************/
void PlainFolder::LoadItems()
{
    mDebug << __func__ << "()"; mDebug.Info();

    Folder::LoadItemsFrom(mBackend.GetFolder(GetID()));
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name)
{
    mDebug << __func__ << "(name:" << name << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(mBackend.CreateFile(GetID(), name));

    std::unique_ptr<File> file(std::make_unique<File>(mBackend, data, *this));

    mItemMap[file->GetName()] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name)
{
    mDebug << __func__ << "(name:" << name << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(mBackend.CreateFolder(GetID(), name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(mBackend, &data, this));

    mItemMap[folder->GetName()] = std::move(folder);
}

/*****************************************************/
void PlainFolder::SubDeleteItem(Item& item)
{
    item.Delete(true);
}

/*****************************************************/
void PlainFolder::SubRenameItem(Item& item, const std::string& newName, bool overwrite)
{
    item.Rename(newName, overwrite, true);
}

/*****************************************************/
void PlainFolder::SubMoveItem(Item& item, Folder& newParent, bool overwrite)
{
    item.Move(newParent, overwrite, true);
}

/*****************************************************/
void PlainFolder::SubDelete()
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.DeleteFolder(GetID());
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& newName, bool overwrite)
{
    mDebug << __func__ << "(name:" << newName << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.RenameFolder(GetID(), newName, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(Folder& newParent, bool overwrite)
{
    mDebug << __func__ << "(parent:" << newParent.GetName() << ")"; mDebug.Info();

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.MoveFolder(GetID(), newParent.GetID(), overwrite);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
