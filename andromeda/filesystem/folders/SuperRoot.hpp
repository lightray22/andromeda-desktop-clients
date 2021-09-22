
#ifndef LIBA2_SUPERROOT_H_
#define LIBA2_SUPERROOT_H_

#include "../Folder.hpp"
#include "Utilities.hpp"

/** A folder that lists filesystems/etc. */
class SuperRoot : public Folder
{
public:

    SuperRoot(Backend& backend);

protected:

    virtual void LoadItems() override;

private:

    Debug debug;
};

#endif