
#include <filesystem>
#include <QtCore/QStandardPaths>

#include "MountContext.hpp"

#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;
#include "andromeda/filesystem/folders/SuperRoot.hpp"
using Andromeda::Filesystem::Folders::SuperRoot;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;
#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

namespace fs = std::filesystem;

/*****************************************************/
MountContext::MountContext(BackendImpl& backend, bool home, std::string mountPath, FuseOptions& options) : 
    mDebug("MountContext",this) 
{
    mDebug << __func__ << "(mountPath:" << mountPath << ")"; mDebug.Info();

    if (home) mountPath = InitHomeRoot()+"/"+mountPath;

    try
    {
        if (fs::exists(mountPath))
        {
            if (!fs::is_directory(mountPath) || !fs::is_empty(mountPath))
                throw NonEmptyMountException(mountPath);
        #if WIN32
            // Windows auto-creates the directory and fails if it already exists
            fs::remove(mountPath);
        #endif
        }
    #if !WIN32 // Linux complains if the directory doesn't exist before mounting
        else if (home) fs::create_directory(mountPath);
    #endif
    }
    catch (const fs::filesystem_error& err)
    {
        mDebug << __func__ << "... " << err.what(); mDebug.Error();
        throw FilesystemErrorException(err);
    }

    mRootFolder = std::make_unique<SuperRoot>(backend);

    mFuseAdapter = std::make_unique<FuseAdapter>(
        mountPath, *mRootFolder, options, FuseAdapter::RunMode::THREAD);
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
    return mFuseAdapter->GetMountPath();
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
