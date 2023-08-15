#ifndef LIBA2_PLAINFOLDER_H_
#define LIBA2_PLAINFOLDER_H_

#include <string>
#include <memory>
#include "nlohmann/json_fwd.hpp"

#include "andromeda/Debug.hpp"
#include "andromeda/filesystem/Folder.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
namespace Folders {

/** A regular Andromeda folder */
class PlainFolder : public Folder
{
public:

    ~PlainFolder() override = default;

    /**
     * Load from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     * @throws BackendException on backend errors
     */
    static std::unique_ptr<PlainFolder> LoadByID(Backend::BackendImpl& backend, const std::string& id);
    
    /** 
     * Construct with JSON data, load fsConfig
     * @param backend reference to backend
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     * @param parent pointer to parent
     * @throws BackendException on backend errors
     */
    PlainFolder(Backend::BackendImpl& backend, const nlohmann::json& data, bool haveItems, Folder* parent);

protected:
    
    /** 
     * Construct without JSON data
     * @param backend reference to backend
     * @param parent pointer to parent
     */
    PlainFolder(Backend::BackendImpl& backend, Folder* parent);
    
    /** 
     * Construct with JSON data without items and NO fsConfig
     * @param backend reference to backend
     * @param data json data from backend
     * @param parent pointer to parent
     * @throws BackendImpl::JSONErrorException on JSON errors
     */
    PlainFolder(Backend::BackendImpl& backend, const nlohmann::json& data, Folder* parent);

    void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) override;

    /** 
     * Populates the item list with items using the given files/folders JSON, calling SyncContents
     * @throws BackendImpl::JSONErrorException on JSON errors
     * @throws BackendException on backend errors
     */
    void LoadItemsFrom(const nlohmann::json& data, ItemLockMap& itemsLocks, const SharedLockW& thisLock);

    void SubCreateFile(const std::string& name, const SharedLockW& thisLock) override;

    void SubCreateFolder(const std::string& name, const SharedLockW& thisLock) override;

    void SubDelete(const DeleteLock& deleteLock) override;

    void SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite) override;

    void SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite) override;

private:

    mutable Debug mDebug;
};

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_PLAINFOLDER_H_
