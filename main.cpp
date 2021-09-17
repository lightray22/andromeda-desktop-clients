#include <iostream>
#include <memory>

#include "A2Fuse.hpp"
#include "Options.hpp"

#include "CLIBackend.hpp"
#include "HTTPBackend.hpp"
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

    std::unique_ptr<Backend> backend;
    switch (options.GetApiType())
    {
        case Options::ApiType::API_URL:
        {
            backend = std::make_unique<HTTPBackend>(
                options.GetApiHostname(), options.GetApiPath()); break;
        }
        case Options::ApiType::API_PATH:
        {
            backend = std::make_unique<CLIBackend>(
                options.GetApiPath()); break;
        }
    }

    try
    {
        backend->Initialize();

        if (options.HasUsername())
            backend->Authenticate(options.GetUsername());
    }
    catch (const Utilities::Exception& ex)
    {
        cout << ex.what() << endl;
        exit(static_cast<int>(ExitCode::BACKEND_INIT));
    }
    
    // TODO construct a BaseFolder with the back end
    // TODO construct and run A2Fuse with a BaseFolder

    debug.Print("exiting!");

    return static_cast<int>(ExitCode::SUCCESS);
}
