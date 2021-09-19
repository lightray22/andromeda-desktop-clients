#include <iostream>
#include <memory>

#include "Options.hpp"
#include "FuseWrapper.hpp"

#include "CLIRunner.hpp"
#include "HTTPRunner.hpp"
#include "Utilities.hpp"

#include "filesystem/BaseFolder.hpp"
#include "filesystem/folders/Folder.hpp"
#include "filesystem/folders/SuperRoot.hpp"

using namespace std;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT,
    FUSE_INIT
};

#define VERSION "0.1-alpha"

int main(int argc, char** argv)
{
    Debug debug("main"); Options options;

    try
    {
        options.Parse(argc, argv);
    }
    catch (const Options::ShowHelpException& ex)
    {
        cout << Options::HelpText() << endl;
        FuseWrapper::ShowHelpText();
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        cout << "version: " << VERSION << endl;
        cout << "a2lib-version: " << A2LIBVERSION << endl;
        FuseWrapper::ShowVersionText();
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        cout << ex.what() << endl;
        cout << Options::HelpText() << endl;
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
    std::unique_ptr<BaseFolder> folder;

    try
    {
        backend.Initialize();

        if (options.HasUsername())
            backend.AuthInteractive(options.GetUsername());

        switch (options.GetMountItemType())
        {
            case Options::ItemType::SUPERROOT:
            {
                folder = std::make_unique<SuperRoot>(backend); break;
            }
            case Options::ItemType::FOLDER:
            {
                folder = std::make_unique<Folder>(backend, options.GetMountItemID()); break;
            }
        }
    }
    catch (const Utilities::Exception& ex)
    {
        cout << ex.what() << endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    try
    {
        FuseWrapper::Start(*folder, options);
    }
    catch (const FuseWrapper::Exception& ex)
    {
        cout << ex.what() << endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    debug.Out("returning...");
    return static_cast<int>(ExitCode::SUCCESS);
}
