
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

namespace fs = std::filesystem;

/*****************************************************/
MountContext::MountContext(bool home, Backend& backend, FuseAdapter::Options& options) : 
    mDebug("MountContext") 
{
    mDebug << __func__ << "(mountPath:" << options.mountPath << ")"; mDebug.Info();

    if (home) options.mountPath = InitHomeRoot()+"/"+options.mountPath;

    try
    {
        if (fs::exists(options.mountPath))
        {
            if (!fs::is_directory(options.mountPath) || !fs::is_empty(options.mountPath))
                throw NonEmptyMountException(options.mountPath);
        #if WIN32
            // Windows auto-creates the directory and fails if it already exists
            fs::remove(options.mountPath);
        #endif
        }
    #if !WIN32 // Linux complains if the directory doesn't exist before mounting
        else if (home) fs::create_directory(options.mountPath);
    #endif
    }
    catch (const fs::filesystem_error& err)
    {
        mDebug << __func__ << "... " << err.what(); mDebug.Error();
        throw FilesystemErrorException(err);
    }

    mRootFolder = std::make_unique<SuperRoot>(backend);

    mFuseAdapter = std::make_unique<FuseAdapter>(
        *mRootFolder, options, FuseAdapter::RunMode::THREAD);
}

/*****************************************************/
MountContext::~MountContext()
{
    mDebug << __func__ << "()"; mDebug.Info();

    const std::string mountPath { GetMountPath() }; // copy

    mFuseAdapter.reset(); // unmount FUSE

    try
    {
        if (!mHomeRoot.empty()) // home-relative mount
        {
            if (fs::is_directory(mountPath) && fs::is_empty(mountPath))
            {
                mDebug << __func__ << "... remove mountPath"; mDebug.Info();
                fs::remove(mountPath);
            }

            if (fs::is_directory(mHomeRoot) && fs::is_empty(mHomeRoot))
            {
                mDebug << __func__ << "... remove homeRoot"; mDebug.Info();
                fs::remove(mHomeRoot);
            }
        }
    }
    catch (const fs::filesystem_error& err)
    {
        mDebug << __func__ << "... " << err.what(); mDebug.Error();
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
    
    try
    {
        if (!fs::is_directory(mHomeRoot))
            fs::create_directory(mHomeRoot);
    }
    catch (const fs::filesystem_error& err)
    {
        mDebug << __func__ << "... " << err.what(); mDebug.Error();
        throw FilesystemErrorException(err);
    }

    return mHomeRoot;
}
