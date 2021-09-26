
#ifndef LIBA2_SUPERROOT_H_
#define LIBA2_SUPERROOT_H_

#include "../Folder.hpp"
#include "Utilities.hpp"

/** A folder that lists filesystems, shared files, etc. */
class SuperRoot : public Folder
{
public:

    /** Exception indicating that subitems cannot be created  */
    class ModifyException : public Exception { public:
        ModifyException() : Exception("Cannot modify superroot") {}; };

    SuperRoot(Backend& backend);
    
    virtual ~SuperRoot(){};

    virtual void Delete(bool real = false) override { throw ModifyException(); }

protected:

    virtual void LoadItems();

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubRemoveItem(Item& item) override { throw ModifyException(); }

private:

    Debug debug;
};

#endif
