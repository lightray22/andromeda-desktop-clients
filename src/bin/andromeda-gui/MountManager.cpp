
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
}

/*****************************************************/
std::string MountManager::InitHomeRoot()
{
    mDebug << __func__ << "()"; mDebug.Info();

    // TODO move to QtUtilities? want to avoid Qt deps outside the windows folder
    QStringList locations { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };

    // TODO throw exception if list is empty
    mHomeRoot = locations[0].toStdString()+"/Andromeda";

    // TODO check for exceptions thrown by these filesystem functions

    #if WIN32
        // Windows auto-creates the directory and fails if it already exists
        if (std::filesystem::is_directory(mHomeRoot))
        {
            if (std::filesystem::is_empty(mHomeRoot)) 
                std::filesystem::remove(mHomeRoot);
        }
    #else
        // Linux complains if the directory doesn't exist before mounting
        if (!std::filesystem::is_directory(mHomeRoot))
            std::filesystem::create_directory(mHomeRoot);
    #endif
    
    // TODO if mount is not empty, error out...

    mDebug << __func__ << "... return " << mHomeRoot; mDebug.Info();

    return mHomeRoot;
}

/*****************************************************/
void MountManager::CreateMount(Backend& backend, FuseAdapter::Options& options)
{
    mDebug << __func__ << "(path:" << options.mountPath << ")"; mDebug.Info();

    if (options.mountPath.empty())
    {
        options.mountPath = InitHomeRoot(); // TODO what if already init'd?

        mDebug << __func__ << "... mountPath:" << options.mountPath << ")"; mDebug.Info();
    }

    // TODO check if already in map? // mMounts.find

    MountContext context;
    
    context.rootFolder = std::make_unique<SuperRoot>(backend);

    context.fuseAdapter = std::make_unique<FuseAdapter>(
        *context.rootFolder, options, FuseAdapter::RunMode::THREAD);

    mMounts.emplace(options.mountPath, std::move(context));
}

/*****************************************************/
void MountManager::RemoveMount() // TODO what arguments?
{
    mDebug << __func__ << "()"; mDebug.Info();

    mMounts.clear(); // TODO for now, remove all

    if (!mHomeRoot.empty() && 
        std::filesystem::exists(mHomeRoot) &&
        std::filesystem::is_empty(mHomeRoot))
    {
        std::filesystem::remove(mHomeRoot);
    }
}
