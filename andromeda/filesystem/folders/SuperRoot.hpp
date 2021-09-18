
#ifndef LIBA2_SUPERROOT_H_
#define LIBA2_SUPERROOT_H_

#include "../BaseFolder.hpp"
#include "Utilities.hpp"

/** A folder that lists filesystems/etc. */
class SuperRoot : public BaseFolder
{
public:

    SuperRoot(Backend& backend);

private:

    Debug debug;
};

#endif