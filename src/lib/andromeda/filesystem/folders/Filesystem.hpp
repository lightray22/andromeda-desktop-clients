
#ifndef LIBA2_FILESYSTEM_H_
#define LIBA2_FILESYSTEM_H_

#include <memory>
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

    virtual ~Filesystem(){};

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

    virtual void Refresh(const nlohmann::json& data, const Andromeda::SharedLockW& itemLock) override { }

protected:

    virtual const std::string& GetID() override;

    virtual void SubLoadItems() override;

    virtual void SubDelete(const SharedLockW& itemLock) override { throw ModifyException(); }

    virtual void SubMove(const std::string& parentID, const SharedLockW& itemLock, bool overwrite = false) override { throw ModifyException(); }

private:

    std::string mFsid;

    Debug mDebug;
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif // LIBA2_FILESYSTEM_H_
