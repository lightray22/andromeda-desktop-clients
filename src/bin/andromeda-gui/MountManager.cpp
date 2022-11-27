
#include <filesystem>
#include <QtCore/QStandardPaths>

#include "MountManager.hpp"

#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda/fsitems/Folder.hpp"
using Andromeda::FSItems::Folder;
#include "andromeda/fsitems/folders/SuperRoot.hpp"
using Andromeda::FSItems::Folders::SuperRoot;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

/*****************************************************/
MountManager::MountManager() : 
    mDebug("MountManager") 
{ 
    mDebug << __func__ << "()"; mDebug.Info();
}

/*****************************************************/
MountManager::~MountManager()
{
    mDebug << __func__ << "()"; mDebug.Info();
    
    mMounts.clear(); // unmount all

    if (!mHomeRoot.empty() && 
        std::filesystem::exists(mHomeRoot) &&
        std::filesystem::is_empty(mHomeRoot))
    {
        std::filesystem::remove(mHomeRoot);
    }
}

/*****************************************************/
std::string MountManager::GetHomeRoot(const std::string& path)
{
    mDebug << __func__ << "(path:" << path << ")"; mDebug.Info();

    if (mHomeRoot.empty())
    {
        // TODO move to QtUtilities? want to avoid Qt deps outside the gui folder
        QStringList locations { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };

        // TODO throw exception if list is empty
        mHomeRoot = locations[0].toStdString()+"/Andromeda";

        // TODO check for exceptions thrown by all filesystem functions

        if (!std::filesystem::is_directory(mHomeRoot))
            std::filesystem::create_directory(mHomeRoot);
    }

    std::string retval { mHomeRoot + "/" + path };

    mDebug << __func__ << "... retval:" << retval; mDebug.Info();

    return retval;
}

/*****************************************************/
void MountManager::CreateMount(bool home, Backend& backend, FuseAdapter::Options& options)
{
    mDebug << __func__ << "(mountPath:" << options.mountPath << ")"; mDebug.Info();

    if (home) options.mountPath = GetHomeRoot(options.mountPath);

    // TODO check if already in map? // mMounts.find

    #if WIN32
        // Windows auto-creates the directory and fails if it already exists
        if (std::filesystem::is_directory(options.mountPath) &&
            std::filesystem::is_empty(options.mountPath))
        {
            std::filesystem::remove(options.mountPath);
        }
    #else
        // Linux complains if the directory doesn't exist before mounting
        if (home && !std::filesystem::is_directory(options.mountPath))
        {
            std::filesystem::create_directory(options.mountPath);
        }
    #endif
    
    // TODO if mount exists and is not empty, error out...

    MountContext context;
    
    context.rootFolder = std::make_unique<SuperRoot>(backend);

    context.fuseAdapter = std::make_unique<FuseAdapter>(
        *context.rootFolder, options, FuseAdapter::RunMode::THREAD);

    mMounts.emplace(options.mountPath, std::move(context));
}

/*****************************************************/
void MountManager::RemoveMount(bool home, std::string mountPath)
{
    mDebug << __func__ << "(mountPath:" << mountPath << ")"; mDebug.Info();

    if (home) mountPath = GetHomeRoot(mountPath);

    MountMap::iterator it { mMounts.find(mountPath) }; // TODO what if not found?

    mMounts.erase(it);

    if (home && std::filesystem::is_directory(mountPath) &&
        std::filesystem::is_empty(mountPath))
    {
        std::filesystem::remove(mountPath);
    }
}
