
#ifndef LIBA2_FILESYSTEMS_H_
#define LIBA2_FILESYSTEMS_H_

#include "../Folder.hpp"

/** A folder that lists filesystems */
class Filesystems : public Folder
{
public:

    /** Exception indicating that subitems cannot be created/deleted */
    class ModifyException : public Exception { public:
        ModifyException() : Exception("Cannot modify filesystems") {}; };

    Filesystems(Backend& backend, Folder& parent);
    
    virtual ~Filesystems(){};

    virtual void Delete(bool real = false) override { throw ModifyException(); }

protected:

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubRemoveItem(Item& item) override { throw ModifyException(); }

private:

    Debug debug;
};

#endif
