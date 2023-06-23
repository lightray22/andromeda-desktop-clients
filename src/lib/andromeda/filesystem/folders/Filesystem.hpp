
#ifndef LIBA2_FILESYSTEM_H_
#define LIBA2_FILESYSTEM_H_

#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "PlainFolder.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
namespace Folders {

/** A root folder accessed by its filesystem ID */
class Filesystem : public PlainFolder
{
public:

    ~Filesystem() override = default;

    /**
     * Load a filesystem from the backend with the given ID
     * @param backend reference to backend
     * @param fsid ID of filesystem to load
     */
    static std::unique_ptr<Filesystem> LoadByID(Backend::BackendImpl& backend, const std::string& fsid);

    /** 
     * Construct with root folder JSON data
     * @param backend reference to backend
     * @param data pre-loaded JSON data
     * @param parent optional pointer to parent
     */
    Filesystem(Backend::BackendImpl& backend, const nlohmann::json& data, Folder* parent);

    void Refresh(const nlohmann::json& data, const Andromeda::SharedLockW& thisLock) override { } // TODO probably need to do something here...?

protected:

    const std::string& GetID() override;

    using UniqueLock = std::unique_lock<std::mutex>;
    /** Sets the folder ID from the given backend data */
    virtual void LoadID(const nlohmann::json& data, const UniqueLock& idLock);

    void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) override;

    void SubDelete(const DeleteLock& deleteLock) override { throw ModifyException(); }

    void SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite = false) override { throw ModifyException(); }

private:

    std::string mFsid;
    std::mutex mIdMutex; // ID is lazy-loaded

    mutable Debug mDebug;
};

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FILESYSTEM_H_
