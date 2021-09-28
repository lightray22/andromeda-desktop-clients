
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

protected:

    virtual void LoadItems() override;

    virtual void SubCreateFile(const std::string& name) override { throw ModifyException(); }

    virtual void SubCreateFolder(const std::string& name) override { throw ModifyException(); }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& name, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug debug;
};

#endif
