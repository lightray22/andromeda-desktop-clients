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
    BACKEND_INIT
};

int main(int argc, char** argv)
{
    Debug debug("main"); Options options;

    try
    {
        options.Parse(argc, argv);
    }
    catch (const Options::Exception& ex)
    {
        cout << ex.what() << endl;
        cout << Options::HelpText() << endl;
        exit(static_cast<int>(ExitCode::BAD_USAGE));
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
        exit(static_cast<int>(ExitCode::BACKEND_INIT));
    }

    FuseWrapper::Start(*folder, options.GetMountPath());

    debug.Out("returning...");

    return static_cast<int>(ExitCode::SUCCESS);
}
