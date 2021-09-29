
#ifndef LIBA2_SUPERROOT_H_
#define LIBA2_SUPERROOT_H_

#include "../Folder.hpp"
#include "Utilities.hpp"

/** A folder that lists filesystems, shared files, etc. */
class SuperRoot : public Folder
{
public:

    SuperRoot(Backend& backend);
    
    virtual ~SuperRoot(){};

protected:

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubDeleteItem(Item& item) override { throw ModifyException(); }

    virtual void SubRenameItem(Item& item, const std::string& name, bool overwrite) override { throw ModifyException(); }

    virtual void SubMoveItem(Item& item, Folder& parent, bool overwrite) override { throw ModifyException(); }

    virtual bool CanReceiveItems() override { return false; }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& name, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(Folder& parent, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug debug;
};

#endif
