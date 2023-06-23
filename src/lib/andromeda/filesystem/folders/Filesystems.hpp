
#ifndef LIBA2_FILESYSTEMS_H_
#define LIBA2_FILESYSTEMS_H_

#include "andromeda/filesystem/Folder.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/** A special folder that lists filesystems */
class Filesystems : public Folder
{
public:

    /**
     * @param backend backend reference
     * @param parent parent folder reference
     */
    Filesystems(Backend::BackendImpl& backend, Folder& parent);
    
    ~Filesystems() override = default;

protected:

    void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) override;

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

#endif // LIBA2_FILESYSTEMS_H_
