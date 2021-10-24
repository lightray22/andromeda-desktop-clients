#include <iostream>
#include <memory>

#include "Options.hpp"
#include "FuseWrapper.hpp"

#include "CLIRunner.hpp"
#include "HTTPRunner.hpp"
#include "Utilities.hpp"

#include "filesystem/Folder.hpp"
#include "filesystem/folders/PlainFolder.hpp"
#include "filesystem/folders/Filesystem.hpp"
#include "filesystem/folders/SuperRoot.hpp"

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
    
    Options options;
    Config::Options bOptions;

    try
    {
        options.Parse(argc, argv, bOptions);
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
        std::cout << ex.what() << std::endl;
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
                options.GetApiHostname(), options.GetApiPath()); break;
        }
        case Options::ApiType::API_PATH:
        {
            runner = std::make_unique<CLIRunner>(
                options.GetApiPath()); break;
        }
    }

    Backend backend(*runner);
    std::unique_ptr<Folder> folder;

    try
    {
        backend.Initialize(bOptions);

        if (options.HasUsername())
        {
            backend.AuthInteractive(options.GetUsername());
        }

        switch (options.GetMountItemType())
        {
            case Options::ItemType::SUPERROOT:
            {
                folder = std::make_unique<SuperRoot>(backend); break;
            }
            case Options::ItemType::FILESYSTEM:
            {
                folder = Filesystem::LoadByID(backend, options.GetMountItemID()); break;
            }
            case Options::ItemType::FOLDER:
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

    try
    {
        FuseWrapper::Start(*folder, options);
    }
    catch (const FuseWrapper::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    debug.Info("returning...");
    return static_cast<int>(ExitCode::SUCCESS);
}
