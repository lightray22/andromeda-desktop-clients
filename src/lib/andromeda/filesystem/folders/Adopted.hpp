
#ifndef LIBA2_ADOPTED_H_
#define LIBA2_ADOPTED_H_

#include "PlainFolder.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class Folder;

namespace Folders {

/** A special folder listing items owned but residing in other users' folders */
class Adopted : public PlainFolder
{
public:

    ~Adopted() override = default;

    /**
     * @param backend backend reference
     * @param parent reference to parent 
     */
    Adopted(Backend::BackendImpl& backend, Folder& parent);

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

#endif // LIBA2_ADOPTED_H_
