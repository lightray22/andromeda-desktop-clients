
#include <iostream>

#include <nlohmann/json.hpp>

#include "CommandLine.hpp"
#include "Options.hpp"

#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/backend/BaseRunner.hpp"
using Andromeda::Backend::BaseRunner;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;

#define VERSION "0.1-alpha"

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    ENDPOINT,
    BACKEND_JSON,
    BACKEND_RESP
};

int main(int argc, char** argv, char** env) // TODO env vars
{
    Debug debug("main");
    
    HTTPOptions httpOptions;
    httpOptions.followRedirects = false;

    Options options(httpOptions);
    CommandLine commandLine(options);

    try
    {
        commandLine.ParseArgs(static_cast<size_t>(argc), argv);

        options.Validate();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << CommandLine::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << VERSION << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << CommandLine::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    Debug::SetLevel(options.GetDebugLevel());

    debug << __func__ << "()"; debug.Info();

    HTTPRunner::HostUrlPair urlPair {
        HTTPRunner::ParseURL(options.GetApiUrl()) };

    HTTPRunner runner(urlPair.first, urlPair.second, httpOptions);

    try
    {
        std::string resp { runner.RunAction(commandLine.GetRunnerInput()) };

        // TODO how to tell if response is JSON or not?

        try
        {
            nlohmann::json val(nlohmann::json::parse(resp));
            std::cout << val.dump(4) << std::endl;

            if (val.at("ok").get<bool>())
            {
                debug.Info("returning success...");
                return static_cast<int>(ExitCode::SUCCESS);
            }
            else
            {
                debug.Info("returning API error...");
                return static_cast<int>(ExitCode::BACKEND_RESP);
            }
        }

        catch (const nlohmann::json::exception& ex)
        {
            std::cerr << "JSON Error: " << ex.what() << std::endl;
            debug << "... json body: " << resp; debug.Error();
            return static_cast<int>(ExitCode::BACKEND_JSON);
        }
    }
    catch (const BaseRunner::EndpointException& ex)
    {
        std::cerr << "HTTP Error: " << ex.what() << std::endl;
        return static_cast<int>(ExitCode::ENDPOINT);
    }
}
