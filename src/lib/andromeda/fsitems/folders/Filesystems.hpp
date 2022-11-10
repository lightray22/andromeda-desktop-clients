
#ifndef LIBA2_FILESYSTEMS_H_
#define LIBA2_FILESYSTEMS_H_

#include "andromeda/fsitems/Folder.hpp"

namespace Andromeda {
namespace FSItems {
namespace Folders {

/** A special folder that lists filesystems */
class Filesystems : public Folder
{
public:

    /**
     * @param backend backend reference
     * @param parent parent folder reference
     */
    Filesystems(Backend& backend, Folder& parent);
    
    virtual ~Filesystems(){};

protected:

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name_) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name_) override { throw ModifyException(); }

    virtual void SubDeleteItem(Item& item) override;

    virtual void SubRenameItem(Item& item, const std::string& newName, bool overwrite) override;

    virtual void SubMoveItem(Item& item, Folder& newParent, bool overwrite) override { throw ModifyException(); }
    
    virtual bool isReadOnly() const override { return true; }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& newName, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(Folder& newParent, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug debug;
};

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders

#endif
