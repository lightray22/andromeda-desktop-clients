
#include <iostream>
#include <exception>

#include "A2Fuse.hpp"
#include "Options.hpp"

#include "CLIBackend.hpp"
#include "HTTPBackend.hpp"

using namespace std;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE
};

int main(int argc, char** argv)
{
    Options options{};

    // TODO add config file parsing with <std::filesystem>
    
    try
    {
        options.Initialize(argc, argv);
    }
    catch (const runtime_error& ex)
    {
        cout << ex.what() << endl;
        cout << Options::HelpText() << endl;
        exit((int)ExitCode::BAD_USAGE);
    }

    Backend* backend = nullptr;

    switch (options.GetApiType())
    {
        case Options::ApiType::API_URL:
            backend = new HTTPBackend(options.GetApiLocation(), options.GetUsername()); break;
        case Options::ApiType::API_PATH:
            backend = new CLIBackend(options.GetApiLocation(), options.GetUsername()); break;
    }
    
    // TODO construct a BaseFolder with the back end
    // TODO construct and run A2Fuse with a BaseFolder

    return (int)ExitCode::SUCCESS;
}
