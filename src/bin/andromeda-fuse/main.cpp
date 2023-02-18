
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
using AndromedaFuse::Options;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;
#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/backend/BaseRunner.hpp"
using Andromeda::Backend::BaseRunner;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/ConfigOptions.hpp"
using Andromeda::Backend::ConfigOptions;
#include "andromeda/backend/CLIRunner.hpp"
using Andromeda::Backend::CLIRunner;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;

#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;
#include "andromeda/filesystem/folders/PlainFolder.hpp"
using Andromeda::Filesystem::Folders::PlainFolder;
#include "andromeda/filesystem/folders/Filesystem.hpp"
using Andromeda::Filesystem::Folders::Filesystem;
#include "andromeda/filesystem/folders/SuperRoot.hpp"
using Andromeda::Filesystem::Folders::SuperRoot;
#include "andromeda/filesystem/filedata/CacheManager.hpp"
using Andromeda::Filesystem::Filedata::CacheManager;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT,
    FUSE_INIT
};

int main(int argc, char** argv)
{
    Debug debug("main",nullptr); 
    
    ConfigOptions configOptions;
    HTTPOptions httpOptions;
    FuseOptions fuseOptions;

    Options options(configOptions, httpOptions, fuseOptions);

    try
    {
        options.ParseConfig("andromeda-fuse");

        options.ParseArgs(static_cast<size_t>(argc), argv);

        options.Validate();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << ANDROMEDA_VERSION << std::endl;
        FuseAdapter::ShowVersionText();
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    DDBG_INFO("()");

    std::unique_ptr<BaseRunner> runner;
    switch (options.GetApiType())
    {
        case Options::ApiType::API_URL:
        {
            HTTPRunner::HostUrlPair urlPair { 
                HTTPRunner::ParseURL(options.GetApiPath()) };

            runner = std::make_unique<HTTPRunner>(
                urlPair.first, urlPair.second, httpOptions);
        }; break;
        case Options::ApiType::API_PATH:
        {
            runner = std::make_unique<CLIRunner>(
                options.GetApiPath()); break;
        }; break;
    }

    BackendImpl backend(configOptions, *runner);
    CacheManager cacheMgr(false); // don't start thread yet
    backend.SetCacheManager(&cacheMgr);

    std::unique_ptr<Folder> folder;
    
    try
    {
        backend.Initialize();

        if (options.HasSession())
            backend.PreAuthenticate(options.GetSessionID(), options.GetSessionKey());
        else if (options.HasUsername())
            backend.AuthInteractive(options.GetUsername(), options.GetPassword(), options.GetForceSession());

        switch (options.GetMountRootType())
        {
            case Options::RootType::SUPERROOT:
                folder = std::make_unique<SuperRoot>(backend); break;
            case Options::RootType::FILESYSTEM:
                folder = Filesystem::LoadByID(backend, options.GetMountItemID()); break;
            case Options::RootType::FOLDER:
                folder = PlainFolder::LoadByID(backend, options.GetMountItemID()); break;
        }
    }
    catch (const BaseException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    try { (dynamic_cast<HTTPRunner&>(*runner)).EnableRetry(); }
    catch (const std::bad_cast& ex) { } // no retries during init

    try
    {
        FuseAdapter fuseAdapter(options.GetMountPath(), *folder, fuseOptions);

        // In either case, StartFuse() will block until unmounted
        if (options.isForeground())
        {
            cacheMgr.StartThreads();
            fuseAdapter.StartFuse(
                FuseAdapter::RunMode::FOREGROUND);
        }
        else
        { // daemonize kills threads, start cacheMgr in the callback
            fuseAdapter.StartFuse(
                FuseAdapter::RunMode::DAEMON,
                [&](){ cacheMgr.StartThreads(); });
        }
    }
    catch (const FuseAdapter::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    DDBG_INFO(": returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
