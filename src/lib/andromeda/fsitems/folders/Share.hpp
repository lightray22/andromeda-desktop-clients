
#ifndef LIBA2_SHARE_H_
#define LIBA2_SHARE_H_

#include "andromeda/fsitems/Item.hpp"

namespace Andromeda {
namespace FSItems {
namespace Folders {

/** An item accessed via a share */
class Share : public Item
{
public:

    virtual ~Share(){};
};

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders

#endif
