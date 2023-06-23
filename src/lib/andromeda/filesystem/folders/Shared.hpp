
#ifndef LIBA2_SHARED_H_
#define LIBA2_SHARED_H_

#include "PlainFolder.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/** A special folder showing the user's shared items */
class Shared : public PlainFolder
{
public:
    
    ~Shared() override = default;
};

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_SHARED_H_
