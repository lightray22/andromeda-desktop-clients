
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

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& newName, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(Folder& newParent, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug mDebug;
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif // LIBA2_SUPERROOT_H_
