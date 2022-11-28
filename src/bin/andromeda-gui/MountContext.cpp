
#include <filesystem>
#include <QtCore/QStandardPaths>

#include "MountContext.hpp"

#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda/fsitems/Folder.hpp"
using Andromeda::FSItems::Folder;
#include "andromeda/fsitems/folders/SuperRoot.hpp"
using Andromeda::FSItems::Folders::SuperRoot;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

// TODO cleanup exists vs is directory
// TODO check for exceptions thrown by all filesystem functions

/*****************************************************/
MountContext::MountContext(bool home, Backend& backend, FuseAdapter::Options& options) : 
    mDebug("MountContext") 
{
    mDebug << __func__ << "(mountPath:" << options.mountPath << ")"; mDebug.Info();

    if (home) options.mountPath = InitHomeRoot()+"/"+options.mountPath;

    #if WIN32
        // Windows auto-creates the directory and fails if it already exists
        if (std::filesystem::is_directory(options.mountPath)
            && std::filesystem::is_empty(options.mountPath))
        {
            std::filesystem::remove(options.mountPath);
        }
    #else
        // Linux complains if the directory doesn't exist before mounting
        if (home && !std::filesystem::is_directory(options.mountPath))
            std::filesystem::create_directory(options.mountPath);
    #endif

    if (std::filesystem::exists(options.mountPath)
        && !std::filesystem::is_empty(options.mountPath))
        throw NonEmptyMountException(options.mountPath);

    mRootFolder = std::make_unique<SuperRoot>(backend);

    mFuseAdapter = std::make_unique<FuseAdapter>(
        *mRootFolder, options, FuseAdapter::RunMode::THREAD);
}

/*****************************************************/
MountContext::~MountContext()
{
    mDebug << __func__ << "()"; mDebug.Info();

    mFuseAdapter.reset(); // unmount FUSE
    
    if (!mHomeRoot.empty() && 
        std::filesystem::exists(mHomeRoot) &&
        std::filesystem::is_empty(mHomeRoot))
    {
        mDebug << __func__ << "... remove homeRoot"; mDebug.Info();

        std::filesystem::remove(mHomeRoot);
    }
}

/*****************************************************/
const std::string& MountContext::GetMountPath() const
{
    return mFuseAdapter->GetOptions().mountPath;
}

/*****************************************************/
const std::string& MountContext::InitHomeRoot()
{
    mDebug << __func__ << "()"; mDebug.Info();

    QStringList locations { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };
    if (locations.empty()) throw UnknownHomeException();

    mHomeRoot = locations.at(0).toStdString()+"/Andromeda";

    mDebug << __func__ << "... homeRoot:" << mHomeRoot; mDebug.Info();

    
    if (!std::filesystem::is_directory(mHomeRoot))
        std::filesystem::create_directory(mHomeRoot);

    return mHomeRoot;
}
