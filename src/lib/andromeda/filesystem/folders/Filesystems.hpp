
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
    
    virtual ~Filesystems(){};

protected:

    virtual void SubLoadItems() override;

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& newName, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(const std::string& parentID, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug mDebug;
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif // LIBA2_FILESYSTEMS_H_
