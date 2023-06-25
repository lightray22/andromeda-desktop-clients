
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
MountContext::MountContext(BackendImpl& backend, bool homeRel, std::string mountPath, FuseOptions& options) : 
    mHomeRelative(homeRel), mDebug(__func__,this) 
{
    MDBG_INFO("(mountPath:" << mountPath << ")");

    if (mHomeRelative)
    {
        const QStringList locations { QStandardPaths::standardLocations(QStandardPaths::HomeLocation) };
        if (locations.empty()) throw UnknownHomeException();

        mountPath = locations.at(0).toStdString()+"/"+mountPath;
        MDBG_INFO("... mountPath:" << mountPath);
    }

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
        else if (mHomeRelative)
            fs::create_directory(mountPath);
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
        if (mHomeRelative && fs::is_directory(mountPath) && fs::is_empty(mountPath))
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
