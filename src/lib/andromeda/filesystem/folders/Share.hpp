
#ifndef LIBA2_SHARE_H_
#define LIBA2_SHARE_H_

#include "andromeda/filesystem/Item.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/** An item accessed via a share */
class Share : public Item
{
public:

    virtual ~Share(){};
};

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders

#endif // LIBA2_SHARE_H_
