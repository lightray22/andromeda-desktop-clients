#ifndef LIBA2_PLAINFOLDER_H_
#define LIBA2_PLAINFOLDER_H_

#include <string>
#include <memory>
#include <nlohmann/json_fwd.hpp>

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

    virtual ~PlainFolder(){};

    /**
     * Load from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     */
    static std::unique_ptr<PlainFolder> LoadByID(Backend::BackendImpl& backend, const std::string& id);
    
    /** 
     * Construct with JSON data, load fsConfig
     * @param backend reference to backend
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     * @param parent pointer to parent
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
     */
    PlainFolder(Backend::BackendImpl& backend, const nlohmann::json& data, Folder* parent);

    virtual void SubLoadItems() override;

    /** Populate/merge itemMap using the given files/folders JSON */
    virtual void LoadItemsFrom(const nlohmann::json& data);

    virtual void SubCreateFile(const std::string& name, const SharedLockW& itemLock) override;

    virtual void SubCreateFolder(const std::string& name, const SharedLockW& itemLock) override;

    virtual void SubDelete(const SharedLockW& itemLock) override;

    virtual void SubRename(const std::string& newName, const SharedLockW& itemLock, bool overwrite) override;

    virtual void SubMove(const std::string& parentID, const SharedLockW& itemLock, bool overwrite) override;

private:

    Debug mDebug;
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif // LIBA2_PLAINFOLDER_H_
