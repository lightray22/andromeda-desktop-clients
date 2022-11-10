
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

#include "andromeda/CLIRunner.hpp"
using Andromeda::CLIRunner;
#include "andromeda/HTTPRunner.hpp"
using Andromeda::HTTPRunner;
#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda/Config.hpp"
using Andromeda::Config;
#include "andromeda/Utilities.hpp"
using Andromeda::Debug;
using Andromeda::Utilities;

#include "andromeda/fsitems/Folder.hpp"
using Andromeda::FSItems::Folder;
#include "andromeda/fsitems/folders/PlainFolder.hpp"
using Andromeda::FSItems::Folders::PlainFolder;
#include "andromeda/fsitems/folders/Filesystem.hpp"
using Andromeda::FSItems::Folders::Filesystem;
#include "andromeda/fsitems/folders/SuperRoot.hpp"
using Andromeda::FSItems::Folders::SuperRoot;

#define VERSION "0.1-alpha"

using AndromedaFuse::FuseAdapter;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT,
    FUSE_INIT
};

int main(int argc, char** argv)
{
    Debug debug("main"); 
    
    Config::Options cOptions;
    HTTPRunner::Options hOptions;
    FuseAdapter::Options fOptions;

    Options options(cOptions, hOptions, fOptions);

    try
    {
        std::list<std::string> paths { 
            "/etc/andromeda", "/usr/local/etc/andromeda" };

        #if WIN32
            #pragma warning(push)
            #pragma warning(disable:4996) // getenv is safe in C++11
        #endif

        const char* homeDir = std::getenv("HOME");
        if (homeDir != nullptr) paths.push_back(
            std::string(homeDir)+"/.config/andromeda");

        #if WIN32
            #pragma warning(pop)
        #endif

        paths.push_back("."); for (std::string path : paths)
        {
            path += "/andromeda-fuse.conf";

            if (std::filesystem::is_regular_file(path))
                options.ParseFile(path);
        }

        options.ParseArgs(argc, argv);

        options.CheckMissing();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << VERSION << std::endl;
        std::cout << "a2lib-version: " << A2LIBVERSION << std::endl;
        FuseAdapter::ShowVersionText();
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    Debug::SetLevel(options.GetDebugLevel());

    std::unique_ptr<Backend::Runner> runner;
    switch (options.GetApiType())
    {
        case Options::ApiType::API_URL:
        {
            runner = std::make_unique<HTTPRunner>(
                options.GetApiHostname(), options.GetApiPath(), hOptions);
        }; break;
        case Options::ApiType::API_PATH:
        {
            runner = std::make_unique<CLIRunner>(
                options.GetApiPath()); break;
        }; break;
    }

    Backend backend(*runner);
    std::unique_ptr<Folder> folder;

    try
    {
        backend.Initialize(cOptions);

        if (options.HasSession())
        {
            backend.PreAuthenticate(options.GetSessionID(), options.GetSessionKey());
        }
        else if (options.HasUsername())
        {
            backend.AuthInteractive(options.GetUsername(), options.GetPassword(), options.GetForceSession());
        }

        switch (options.GetMountRootType())
        {
            case Options::RootType::SUPERROOT:
            {
                folder = std::make_unique<SuperRoot>(backend); break;
            }
            case Options::RootType::FILESYSTEM:
            {
                folder = Filesystem::LoadByID(backend, options.GetMountItemID()); break;
            }
            case Options::RootType::FOLDER:
            {
                folder = PlainFolder::LoadByID(backend, options.GetMountItemID()); break;
            }
        }
    }
    catch (const Utilities::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    try { (dynamic_cast<HTTPRunner&>(*runner)).EnableRetry(); }
    catch (const std::bad_cast& ex) { } // no retries during init

    try
    {
        FuseAdapter fuseAdapter(*folder, fOptions, true);
    }
    catch (const FuseAdapter::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    debug.Info("returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
