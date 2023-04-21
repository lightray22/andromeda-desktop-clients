
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
    
    virtual ~SuperRoot(){};

protected:

    virtual void LoadItems(const SharedLockW& thisLock, bool force = false) override;

    virtual void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) override { }; // unused

    virtual void SubCreateFile(const std::string& name, const SharedLockW& thisLock) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name, const SharedLockW& thisLock) override { throw ModifyException(); }

    virtual void SubDelete(const DeleteLock& deleteLock) override { throw ModifyException(); }

    virtual void SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug mDebug;
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif // LIBA2_SUPERROOT_H_
