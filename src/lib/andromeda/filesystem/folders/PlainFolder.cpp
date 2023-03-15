
#include <nlohmann/json.hpp>

#include "PlainFolder.hpp"
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
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
    Folder(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

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
        
        MDBG_INFO("... ID:" << GetID() << " name:" << mName);
    }
}

/*****************************************************/
void PlainFolder::LoadItems()
{
    ITDBG_INFO("()");

    Folder::LoadItemsFrom(mBackend.GetFolder(GetID()));
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    std::unique_ptr<File> file;

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        nlohmann::json data(mBackend.CreateFile(GetID(), name));
        file = std::make_unique<File>(mBackend, data, *this);
    }
    else file = std::make_unique<File>(mBackend, *this, name, *mFsConfig);

    mItemMap[file->GetName()] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(mBackend.CreateFolder(GetID(), name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(mBackend, &data, this));

    mItemMap[folder->GetName()] = std::move(folder);
}

/*****************************************************/
void PlainFolder::SubDelete()
{
    ITDBG_INFO("()");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.DeleteFolder(GetID());
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& newName, bool overwrite)
{
    ITDBG_INFO("(name:" << newName << ")");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.RenameFolder(GetID(), newName, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(Folder& newParent, bool overwrite)
{
    ITDBG_INFO("(parent:" << newParent.GetName() << ")");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.MoveFolder(GetID(), newParent.GetID(), overwrite);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
