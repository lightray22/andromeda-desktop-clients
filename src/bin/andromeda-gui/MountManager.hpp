#ifndef A2GUI_MOUNTMANAGER_H
#define A2GUI_MOUNTMANAGER_H

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "andromeda/Utilities.hpp"
#include "andromeda/fsitems/Folder.hpp"
#include "andromeda-fuse/FuseAdapter.hpp"

namespace Andromeda { class Backend; }

// TODO comments (here + all functions)
class MountManager
{
public:

    MountManager();

    virtual ~MountManager();

    void CreateMount(Andromeda::Backend& backend, AndromedaFuse::FuseAdapter::Options& options);

    void RemoveMount();

    /** Returns the path to the home root, if mounted */
    std::string GetHomeRoot() { return mHomeRoot; }

private:

    /** Sets up and returns the path to HOMEDIR/Andromeda */
    std::string InitHomeRoot();

    /** The currently mounted standard home mount path */
    std::string mHomeRoot;

    /** A FuseAdapter instance + resources */
    struct MountContext
    {
        std::unique_ptr<Andromeda::FSItems::Folder> rootFolder;
        std::unique_ptr<AndromedaFuse::FuseAdapter> fuseAdapter;
    };

    typedef std::map<std::string, MountContext> MountMap; MountMap mMounts;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_MOUNTMANAGER_H
