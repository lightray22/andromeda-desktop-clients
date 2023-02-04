
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

namespace AndromedaGui {

/*****************************************************/
MountContext::MountContext(BackendImpl& backend, bool home, std::string mountPath, FuseOptions& options) : 
    mDebug("MountContext",this) 
{
    MDBG_INFO("(mountPath:" << mountPath << ")");

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
        #endif // WIN32
        }
    #if !WIN32 // Linux complains if the directory doesn't exist before mounting
        else if (home) fs::create_directory(mountPath);
    #endif // !WIN32
    }
    catch (const fs::filesystem_error& err)
    {
        MDBG_ERROR("... " << err.what());
        throw FilesystemErrorException(err);
    }

    mRootFolder = std::make_unique<SuperRoot>(backend);

    mFuseAdapter = std::make_unique<FuseAdapter>(
        mountPath, *mRootFolder, options);
    
    mFuseAdapter->StartFuse(FuseAdapter::RunMode::THREAD); // background
}

/*****************************************************/
MountContext::~MountContext()
{
    MDBG_INFO("()");

    const std::string mountPath { GetMountPath() }; // copy

    mFuseAdapter.reset(); // unmount before removing dirs

    try
    {
        if (!mHomeRoot.empty()) // home-relative mount
        {
            if (fs::is_directory(mountPath) && fs::is_empty(mountPath))
            {
                MDBG_INFO("... remove mountPath");
                fs::remove(mountPath);
            }

            if (fs::is_directory(mHomeRoot) && fs::is_empty(mHomeRoot))
            {
                MDBG_INFO("... remove homeRoot");
                fs::remove(mHomeRoot);
            }
        }
    }
    catch (const fs::filesystem_error& err)
    {
        MDBG_ERROR("... " << err.what()); // ignore
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
    MDBG_INFO("()");

    QStringList locations { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };
    if (locations.empty()) throw UnknownHomeException();

    mHomeRoot = locations.at(0).toStdString()+"/Andromeda";
    MDBG_INFO("... homeRoot:" << mHomeRoot);
    
    try
    {
        if (!fs::is_directory(mHomeRoot))
            fs::create_directory(mHomeRoot);
    }
    catch (const fs::filesystem_error& err)
    {
        MDBG_ERROR("... " << err.what());
        throw FilesystemErrorException(err);
    }

    return mHomeRoot;
}

} // namespace AndromedaGui
