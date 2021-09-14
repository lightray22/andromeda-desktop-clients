#include <iostream>
#include <memory>

#include "A2Fuse.hpp"
#include "Options.hpp"

#include "CLIRunner.hpp"
#include "HTTPRunner.hpp"
#include "Backend.hpp"

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
        options.Parse(argc, argv);
    }
    catch (const OptionsException& ex)
    {
        cout << ex.what() << endl;
        cout << Options::HelpText() << endl;
        exit(static_cast<int>(ExitCode::BAD_USAGE));
    }

    std::unique_ptr<Runner> runner;
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

    if (options.HasUsername())
        backend.Authenticate(options.GetUsername());
    
    // TODO construct a BaseFolder with the back end
    // TODO construct and run A2Fuse with a BaseFolder

    return static_cast<int>(ExitCode::SUCCESS);
}
