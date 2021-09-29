
#ifndef LIBA2_ADOPTED_H_
#define LIBA2_ADOPTED_H_

#include "PlainFolder.hpp"
#include "Utilities.hpp"

class Backend;
class Folder;

/** Lists items owned but residing in other users' folders */
class Adopted : public PlainFolder
{
public:

    virtual ~Adopted(){};

    /**
     * @param backend backend reference
     * @param parent reference to parent 
     */
    Adopted(Backend& backend, Folder& parent);

protected:

    /** populate itemMap from the backend */
    virtual void LoadItems() override;
        
    virtual bool CanReceiveItems() override { return false; }

    virtual void SubDelete() override { throw ModifyException(); }

    virtual void SubRename(const std::string& name, bool overwrite = false) override { throw ModifyException(); }

    virtual void SubMove(Folder& parent, bool overwrite = false) override { throw ModifyException(); }

private:

    Debug debug;
};

#endif
