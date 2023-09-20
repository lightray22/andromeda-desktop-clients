
#ifndef LIBA2_SUPERROOT_H_
#define LIBA2_SUPERROOT_H_

#include "andromeda/Debug.hpp"
#include "andromeda/filesystem/Folder.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/** A special folder that lists filesystems, shared files, etc. */
class SuperRoot : public Folder
{
public:

    /** @param backend backend reference */
    explicit SuperRoot(Backend::BackendImpl& backend);
    
    ~SuperRoot() override = default;

protected:

    void LoadItems(const SharedLockW& thisLock, bool canRefresh = true) override;

    void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) override { }; // unused

    void SubCreateFile(const std::string& name, const SharedLockW& thisLock) override { throw ModifyException(); }

    void SubCreateFolder(const std::string& name, const SharedLockW& thisLock) override { throw ModifyException(); }

    void SubDelete(const DeleteLock& deleteLock) override { throw ModifyException(); }

    void SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite = false) override { throw ModifyException(); }

    void SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite = false) override { throw ModifyException(); }

private:

    mutable Debug mDebug;
};

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_SUPERROOT_H_
