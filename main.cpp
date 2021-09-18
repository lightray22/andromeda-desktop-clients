#include <iostream>
#include <memory>

#include "A2Fuse.hpp"
#include "Options.hpp"

#include "CLIRunner.hpp"
#include "HTTPRunner.hpp"
#include "Utilities.hpp"

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

    try
    {
        backend.Initialize();

        if (options.HasUsername())
            backend.AuthInteractive(options.GetUsername());

        // TODO construct a BaseFolder with the back end
    }
    catch (const Utilities::Exception& ex)
    {
        cout << ex.what() << endl;
        exit(static_cast<int>(ExitCode::BACKEND_INIT));
    }
    
    // TODO construct and run A2Fuse with a BaseFolder

    debug.Print("exiting!");

    return static_cast<int>(ExitCode::SUCCESS);
}
