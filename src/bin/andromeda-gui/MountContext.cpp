
#include <filesystem>
#include <QtCore/QStandardPaths>

#include "MountContext.hpp"

#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/folders/SuperRoot.hpp"
using Andromeda::Filesystem::Folders::SuperRoot;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;
#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

namespace fs = std::filesystem;

namespace AndromedaGui {

/*****************************************************/
MountContext::MountContext(BackendImpl& backend, bool autoHome, std::string mountPath, FuseOptions& options) : 
    mCreateMount(autoHome), mDebug(__func__,this) 
{
    MDBG_INFO("(mountPath:" << mountPath << ")");

    if (autoHome) // $HOME-relative
    {
        const QStringList locations { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };
        mountPath = locations[0].toStdString()+"/"+mountPath; // Qt guarantees [0] never empty
        MDBG_INFO("... mountPath:" << mountPath);
    }

    try // if mCreateMount, create directory if needed, else must already exist
    {
        if (fs::exists(mountPath))
        {
            if (!fs::is_directory(mountPath) || !fs::is_empty(mountPath))
                throw NonEmptyMountException(mountPath);
        #if WIN32
            // WinFSP mount auto-creates the directory and fails if it already exists
            fs::remove(mountPath);
        #endif // WIN32
        }
        else if (!mCreateMount)
            throw MountNotFoundException(mountPath);
    #if !WIN32 // FUSE mount complains if the directory doesn't exist before mounting
        else fs::create_directory(mountPath);
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

    const std::string mountPath { GetMountPath() }; // copy now
    mFuseAdapter.reset(); // unmount before removing dirs

    try
    {
        if (mCreateMount && fs::is_directory(mountPath))
        {
            MDBG_INFO("... remove mountPath");
            fs::remove(mountPath);
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

} // namespace AndromedaGui
