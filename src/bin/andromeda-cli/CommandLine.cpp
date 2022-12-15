
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
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/RunnerInput.hpp"
using Andromeda::Backend::RunnerInput;
using Andromeda::Backend::RunnerInput_StreamIn;
using Andromeda::Backend::RunnerInput_StreamOut;

/*****************************************************/
std::string CommandLine::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-cli " << Options::CoreHelpText() << endl
        << "andromeda-cli [global flags+] -a|--apiurl url app action [action params+]" << endl
        << "... NOTE a flag without a value cannot be the last argument before app/action" << endl << endl
           
        << "action params: [--$param value] [--$param@ file] [--$param!] [--$param% file [name]] [--$param-]" << endl
        << "         param@ puts the content of the file in the parameter" << endl
        << "         param! will prompt interactively or read stdin for the parameter value" << endl
        << "         param% gives the file path as a direct file input (optionally with a new name)" << endl
        << "         param- will attach the stdin stream as a direct file input" << endl << endl

        << Options::OtherHelpText() << endl;

    return output.str();
}

/*****************************************************/
CommandLine::CommandLine(Options& options) : mOptions(options) { }

/*****************************************************/
CommandLine::~CommandLine() { }

/*****************************************************/
std::string getNextValue(Utilities::StringList& argv, size_t& i)
{
    return (argv.size() > i+1 && argv[i+1][0] != '-') ? argv[++i] : "";
}

/*****************************************************/
void CommandLine::ParseFullArgs(size_t argc, const char* const* argv)
{
    Andromeda::Debug::SetLevel(Andromeda::Debug::Level::DETAILS);

    size_t shift { mOptions.ParseArgs(argc, argv, true) };
    argc -= shift; argv += shift;

    if (argc < 2) throw BaseOptions::BadUsageException("missing app/action");

    std::string app { argv[0] };
    std::string action { argv[1] };

    std::vector<std::string> args;
    for (size_t i = 2; i < argc; i++)
        args.push_back(argv[i]);

    for (const Utilities::StringMap::value_type& pair : Utilities::GetEnvironment())
    {
        if (Utilities::startsWith(pair.first,"andromeda_"))
        {
            const std::string key { Utilities::split(pair.first,"_").second };
            if (!key.empty()) { args.push_back("--"+key); args.push_back(pair.second); }
        }
    }

    RunnerInput::Params params;
    RunnerInput_StreamIn::FileStreams streams;
    RunnerInput_StreamOut::ReadFunc readfunc;

    // see andromeda-server CLI::GetInput()
    for (size_t i = 0; i < args.size(); i++)
    {
        std::string param { args[i] };
        if (param.size() < 2 || !Utilities::startsWith(param,"--"))
            throw BaseOptions::BadUsageException(
                "expected key at action arg "+std::to_string(i));
        else if (param.size() < 3 || std::isspace(param[2]))
            throw BaseOptions::BadUsageException(
                "empty key at action arg "+std::to_string(i));

        param.erase(0,2); const char special { param.back() };

        if (special == '@')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty @ key at action arg "+std::to_string(i));

            std::string val { getNextValue(args, i) };
            if (val.empty()) throw BaseOptions::BadUsageException(
                "expected @ value at action arg "+std::to_string(i));

            if (!std::filesystem::exists(val) || std::filesystem::is_directory(val))
                throw BaseOptions::Exception("Inaccessible file: "+val);

            std::ifstream file(val, std::ios::binary);
            std::string fdat; char buf[4096];
            while (!file.fail())
            {
                file.read(buf, static_cast<std::streamsize>(sizeof(buf)));
                fdat.append(buf, static_cast<std::size_t>(file.gcount()));
            }

            params[param] = fdat;
        }
        else if (special == '!')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty ! key at action arg "+std::to_string(i));

            std::cout << "enter " << param << "..." << std::endl;
            std::string val; std::getline(std::cin, val);

            params[param] = Utilities::trim(val);
        }
        else if (special == '%')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty % key at action arg "+std::to_string(i));
                
            std::string val { getNextValue(args, i) };
            if (val.empty()) throw BaseOptions::BadUsageException(
                "expected % value at action arg "+std::to_string(i));

            if (!std::filesystem::exists(val) || std::filesystem::is_directory(val))
                throw BaseOptions::Exception("Inaccessible file: "+val);

            std::ifstream& file { mOpenFiles.emplace_back(val, std::ios::binary) };

            std::string filename { getNextValue(args, i) }; 
            if (filename.empty()) filename = val;

            streams.emplace(param, RunnerInput_StreamIn::FileStream{ filename, 
                RunnerInput_StreamIn::FromStream(file) });
        }
        else if (special == '-')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty - key at action arg "+std::to_string(i));

            streams.emplace(param, RunnerInput_StreamIn::FileStream{ "data", 
                RunnerInput_StreamIn::FromStream(std::cin) });
        }
        else // plain argument
        {
            params[param] = getNextValue(args, i);
        }
    }

    if (readfunc && !streams.empty())
        throw IncompatibleIOException();

    // TODO add StreamOut CLI option? or maybe just remove StreamOut here?
    // would be simpler + andromeda_download will get used anyway

    if (readfunc)
    {
        mInput_StreamOut = std::make_unique<RunnerInput_StreamOut>(
            RunnerInput_StreamOut{app, action, params, readfunc});
    }
    else if (!streams.empty())
    {
        mInput_StreamIn = std::make_unique<RunnerInput_StreamIn>(
            RunnerInput_StreamIn{app, action, params, { }, streams});
    }
    else
    {
        mInput = std::make_unique<RunnerInput>(
            RunnerInput{app, action, params});
    }
}

/*****************************************************/
std::string CommandLine::RunInputAction(HTTPRunner& runner, bool& isJson)
{
    if (mInput)           return runner.RunAction(*mInput, isJson);
    if (mInput_StreamIn)  return runner.RunAction(*mInput_StreamIn, isJson);
    if (mInput_StreamOut) { runner.RunAction(*mInput_StreamOut, isJson); return ""; }

    throw std::runtime_error("RunInputAction without ParseFullArgs");
}
