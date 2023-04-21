
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

    virtual ~Adopted(){};

    /**
     * @param backend backend reference
     * @param parent reference to parent 
     */
    Adopted(Backend::BackendImpl& backend, Folder& parent);

protected:

    virtual void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) override;
    
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

#endif // LIBA2_ADOPTED_H_
