#include <nlohmann/json.hpp>

#include "Filesystems.hpp"
#include "Filesystem.hpp"
#include "Backend.hpp"

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

    try
    {
        nlohmann::json data(backend.GetFilesystems());

        for (const nlohmann::json& el : data)
        {
            std::unique_ptr<Filesystem> filesystem(Filesystem::LoadFromData(backend, *this, el));

            debug << __func__ << "... filesystem:" << filesystem->GetName(); debug.Info();

            this->itemMap[filesystem->GetName()] = std::move(filesystem);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
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
