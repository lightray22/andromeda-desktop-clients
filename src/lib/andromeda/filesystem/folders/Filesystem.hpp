
#ifndef LIBA2_FILESYSTEM_H_
#define LIBA2_FILESYSTEM_H_

#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "PlainFolder.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
class Backend;

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
    static std::unique_ptr<Filesystem> LoadByID(Backend& backend, const std::string& fsid);

    /** 
     * Construct with root folder JSON data
     * @param backend reference to backend
     * @param data pre-loaded JSON data
     * @param parent optional pointer to parent
     */
    Filesystem(Backend& backend, const nlohmann::json& data, Folder* parent = nullptr);

    virtual const std::string& GetID() override;

    virtual void Refresh(const nlohmann::json& data) override { }

protected:

    virtual void LoadItems() override;

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubMove(Folder& newParent, bool overwrite = false) override { throw ModifyException(); }

private:

    std::string mFsid;

    Debug mDebug;
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif
