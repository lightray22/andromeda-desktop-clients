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

    // TODO need an automatic way to ensure that if a backend is deleted, we don't have a stale reference here
    void CreateMount(bool home, Andromeda::Backend& backend, AndromedaFuse::FuseAdapter::Options& options);

    void RemoveMount(bool home, std::string mountPath);

    /** Sets up and returns the path to HOMEDIR/Andromeda */
    std::string GetHomeRoot(const std::string& path);

private:

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
