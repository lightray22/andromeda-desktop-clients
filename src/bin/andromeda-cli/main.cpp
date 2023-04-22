
#include <iostream>

#include <nlohmann/json.hpp>

#include "CommandLine.hpp"
using AndromedaCli::CommandLine;
#include "Options.hpp"
using AndromedaCli::Options;

#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/backend/BaseRunner.hpp"
using Andromeda::Backend::BaseRunner;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    ENDPOINT,
    BACKEND_JSON,
    BACKEND_RESP
};

#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;

int main(int argc, char** argv)
{
    Debug debug("main",nullptr);
    
    HTTPOptions httpOptions;
    httpOptions.followRedirects = false;

    Options options(httpOptions);
    CommandLine commandLine(options);

    try
    {
        options.ParseConfig("andromeda");
        options.ParseConfig("andromeda-cli");

        commandLine.ParseFullArgs(static_cast<size_t>(argc), argv);

        options.Validate();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << CommandLine::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << ANDROMEDA_VERSION << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << CommandLine::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    DDBG_INFO("()");

    HTTPRunner::HostUrlPair urlPair {
        HTTPRunner::ParseURL(options.GetApiUrl()) };

    HTTPRunner runner(urlPair.first, urlPair.second, httpOptions);

    try
    {
        bool isJson = false; std::string resp { 
            commandLine.RunInputAction(runner, isJson) };

        if (!isJson)
        {
            std::cout << resp;
            return static_cast<int>(ExitCode::SUCCESS);
        }
        else try
        {
            nlohmann::json val(nlohmann::json::parse(resp));
            std::cout << val.dump(4) << std::endl;

            if (val.at("ok").get<bool>())
            {
                DDBG_INFO(": returning success...");
                return static_cast<int>(ExitCode::SUCCESS);
            }
            else
            {
                DDBG_INFO(": returning API error...");
                return static_cast<int>(ExitCode::BACKEND_RESP);
            }
        }
        catch (const nlohmann::json::exception& ex)
        {
            DDBG_ERROR(": JSON Error: " << ex.what());
            DDBG_ERROR("... json body: " << resp);
            return static_cast<int>(ExitCode::BACKEND_JSON);
        }
    }
    catch (const BaseRunner::EndpointException& ex)
    {
        DDBG_ERROR(": HTTP Error: " << ex.what());
        return static_cast<int>(ExitCode::ENDPOINT);
    }
}
