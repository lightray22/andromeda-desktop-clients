#ifndef A2GUI_MOUNTCONTEXT_H
#define A2GUI_MOUNTCONTEXT_H

#include <memory>
#include <string>

#include "andromeda/Utilities.hpp"
#include "andromeda-fuse/FuseAdapter.hpp"

namespace Andromeda { class Backend; 
    namespace FSItems { class Folder; }
}

// TODO comments (here + all functions)
class MountContext
{
public:

    MountContext(bool home, Andromeda::Backend& backend, AndromedaFuse::FuseAdapter::Options& options);

    virtual ~MountContext();

    const std::string& GetMountPath() const;

private:

    /** Sets up and returns the path to HOMEDIR/Andromeda */
    const std::string& InitHomeRoot();

    std::string mHomeRoot;

    std::unique_ptr<Andromeda::FSItems::Folder> mRootFolder;
    std::unique_ptr<AndromedaFuse::FuseAdapter> mFuseAdapter;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_MOUNTCONTEXT_H
