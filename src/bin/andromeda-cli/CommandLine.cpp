
#include <array>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "CommandLine.hpp"
#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/PlatformUtil.hpp"
using Andromeda::PlatformUtil;
#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/RunnerInput.hpp"
using Andromeda::Backend::RunnerInput;
using Andromeda::Backend::RunnerInput_StreamIn;
using Andromeda::Backend::RunnerInput_StreamOut;

namespace AndromedaCli {

/*****************************************************/
std::string CommandLine::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-cli " << Options::CoreHelpText() << endl
        << "andromeda-cli " << Options::MainHelpText() << " -- app action [action params+]" << endl << endl

        << "NOTE that -- always comes before the server action command! This is different than andromeda-server." << endl
        << "NOTE as with the andromeda-server CLI, any action param can be given as an andromeda_key=value environment variable." << endl
        << "NOTE all non-file and non-environment parameters will be sent as URL variables. Use stdin (opt@ or opt!) " << endl
        << "    or environment variables for private data, as they will be sent in the POST body instead." << endl << endl
           
        << "action params: [--$param value] [--$param@ file] [--$param!] [--$param% file [name]] [--$param-]" << endl
        << "         param@ puts the content of the file in the parameter" << endl
        << "         param! will prompt interactively or read stdin for the parameter value" << endl
        << "         param% gives the file path as a direct file input (optionally with a new name)" << endl
        << "         param- will attach the stdin stream as a direct file input" << endl << endl

        << Options::DetailHelpText() << endl;

    return output.str();
}

/*****************************************************/
CommandLine::CommandLine(Options& options, size_t argc, const char* const* argv) : mOptions(options)
{
    const size_t shift { mOptions.ParseArgs(argc, argv, true) };
    argc -= shift; argv += shift; mOptions.Validate();

    if (argc < 2) throw BaseOptions::BadUsageException("missing app/action");

    std::string app { argv[0] };
    std::string action { argv[1] };

    RunnerInput::Params plainParams;
    RunnerInput::Params dataParams;
    RunnerInput_StreamIn::FileStreams inStreams;
    const bool outStream { mOptions.isStreamOut() };

    { // environment params
        StringUtil::StringList args;
        for (const PlatformUtil::StringMap::value_type& pair : PlatformUtil::GetEnvironment("andromeda_"))
        {
            const std::string key { StringUtil::split(pair.first,"_").second };
            if (!key.empty()) { args.push_back("--"+key); args.push_back(pair.second); }
        }
        ProcessArgList(args, true, plainParams, dataParams, inStreams);
    }

    { // command line params
        StringUtil::StringList args;
        for (size_t i = 2; i < argc; i++)
            args.emplace_back(argv[i]);

        ProcessArgList(args, false, plainParams, dataParams, inStreams);
    }

    if (outStream && !inStreams.empty())
        throw IncompatibleIOException();

    if (outStream)
    {
        mInput_StreamOut = std::make_unique<RunnerInput_StreamOut>(
            RunnerInput_StreamOut{{app, action, plainParams, dataParams}, { }});
    }
    else if (!inStreams.empty())
    {
        mInput_StreamIn = std::make_unique<RunnerInput_StreamIn>(
            RunnerInput_StreamIn{{{app, action, plainParams, dataParams}, { }}, inStreams});
    }
    else mInput = std::make_unique<RunnerInput>(
            RunnerInput{app, action, plainParams, dataParams});
}

/*****************************************************/
std::string CommandLine::getNextValue(const StringUtil::StringList& argv, size_t& i)
{
    if (StringUtil::startsWith(argv[i],"--") && 
        argv[i].find('=') != std::string::npos) // --key=val
    {
        return StringUtil::split(argv[i],"=").second;
    }
    
    if (argv.size() > i+1 && !StringUtil::startsWith(argv[i+1],"--")) // --key val
        return argv[++i];
    return ""; // no value
}

/*****************************************************/
void CommandLine::ProcessArgList(const StringUtil::StringList& args, bool isPriv,
    RunnerInput::Params& plainParams, RunnerInput::Params& dataParams, 
    RunnerInput_StreamIn::FileStreams& inStreams)
{
    // see andromeda-server CLI::GetInput()
    for (size_t i = 0; i < args.size(); i++)
    {
        std::string param { args[i] };
        if (param.size() < 2 || !StringUtil::startsWith(param,"--"))
            throw BaseOptions::BadUsageException(
                "expected key at action arg "+std::to_string(i));
        else if (param.size() < 3 || std::isspace(param[2]))
            throw BaseOptions::BadUsageException(
                "empty key at action arg "+std::to_string(i));

        param.erase(0,2); // remove --
        param = StringUtil::split(param,"=").first;
        const char special { param.back() };

        if (special == '@')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty @ key at action arg "+std::to_string(i));

            const std::string val { getNextValue(args, i) };
            if (val.empty()) throw BaseOptions::BadUsageException(
                "expected @ value at action arg "+std::to_string(i));

            if (!std::filesystem::exists(val) || std::filesystem::is_directory(val))
                throw BaseOptions::Exception("Inaccessible file: "+val);

            const std::ifstream file(val, std::ios::binary);
            std::ostringstream fileS; fileS << file.rdbuf(); // read file to string

            // want to overwrite, not emplace
            dataParams[param] = fileS.str();
        }
        else if (special == '!')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty ! key at action arg "+std::to_string(i));

            std::cout << "enter " << param << "..." << std::endl;
            std::string val; std::getline(std::cin, val);

            StringUtil::trim_void(val);
            // want to overwrite, not emplace
            dataParams[param] = val;
        }
        else if (special == '%')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty % key at action arg "+std::to_string(i));
                
            const std::string path { getNextValue(args, i) };
            if (path.empty()) throw BaseOptions::BadUsageException(
                "expected % value at action arg "+std::to_string(i));

            if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
                throw BaseOptions::Exception("Inaccessible file: "+path);

            std::ifstream& file { mOpenFiles.emplace_back(path, std::ios::binary) };

            std::string filename { getNextValue(args, i) }; 
            if (filename.empty()) filename = StringUtil::splitPath(path).second;

            inStreams.emplace(param, RunnerInput_StreamIn::FileStream{ filename, 
                RunnerInput_StreamIn::FromStream(file) });
        }
        else if (special == '-')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty - key at action arg "+std::to_string(i));

            std::string filename { getNextValue(args, i) }; 
            if (filename.empty()) filename = "data";

            inStreams.emplace(param, RunnerInput_StreamIn::FileStream{ filename, 
                RunnerInput_StreamIn::FromStream(std::cin) });
        }
        else // plain argument
        {
            if (!isPriv && !mOptions.AllowUnsafeUrl() && 
                (param.find("password") != std::string::npos || param.find("auth_") != std::string::npos))
                throw PrivateDataException(param); // hardcoded sanity check for now...

            std::string next { getNextValue(args, i) }; // non-const for move
            // want to overwrite, not emplace
            if (isPriv) dataParams[param] = std::move(next);
            else plainParams[param] = std::move(next);
        }
    }
}

/*****************************************************/
std::string CommandLine::RunInputAction(HTTPRunner& runner, bool& isJson, const Andromeda::Backend::ReadFunc& streamOut)
{
    if (mInput)
        // no way to tell read/write via CLI so just assume write
        return runner.RunAction_Write(*mInput, isJson);
    else if (mInput_StreamIn)
        return runner.RunAction_StreamIn(*mInput_StreamIn, isJson);
    else if (mInput_StreamOut)
    { 
        mInput_StreamOut->streamer = streamOut;
        runner.RunAction_StreamOut(*mInput_StreamOut, isJson);
        isJson = false; return "";
    }
    else throw std::runtime_error("no mInput to run"); // can't happen
}

} // namespace AndromedaCli
