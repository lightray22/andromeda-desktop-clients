
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
#include "FuseWrapper.hpp"

#include "CLIRunner.hpp"
#include "HTTPRunner.hpp"
#include "Utilities.hpp"

#include "fsitems/Folder.hpp"
#include "fsitems/folders/PlainFolder.hpp"
#include "fsitems/folders/Filesystem.hpp"
#include "fsitems/folders/SuperRoot.hpp"

#define VERSION "0.1-alpha"

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
    FuseWrapper::Options fOptions;
    HTTPRunner::Options hOptions;

    Options options(cOptions, fOptions, hOptions);

    try
    {
        std::list<std::string> paths { 
            "/etc/andromeda", "/usr/local/etc/andromeda" };

        const char* homeDir = std::getenv("HOME");
        if (homeDir != nullptr) paths.push_back(
            std::string(homeDir)+"/.config/andromeda");

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
        FuseWrapper::ShowVersionText();
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
        FuseWrapper::Start(*folder, fOptions);
    }
    catch (const FuseWrapper::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    debug.Info("returning...");
    return static_cast<int>(ExitCode::SUCCESS);
}
