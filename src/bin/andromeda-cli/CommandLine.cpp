
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "CommandLine.hpp"
#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda/backend/RunnerInput.hpp"
using Andromeda::Backend::RunnerInput;

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

    mInput.app = argv[0];
    mInput.action = argv[1];

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
            typedef std::istreambuf_iterator<char> FStreamIt;
            std::string fdat((FStreamIt(file)), (FStreamIt()));

            mInput.params[param] = Utilities::trim(fdat);
        }
        else if (special == '!')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty ! key at action arg "+std::to_string(i));

            std::cout << "enter " << param << "..." << std::endl;
            std::string val; std::getline(std::cin, val);

            mInput.params[param] = Utilities::trim(val);
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

            mInput.sfiles.emplace(param, RunnerInput::FileStream{ filename, file });
        }
        else if (special == '-')
        {
            param.pop_back(); if (param.empty()) throw BaseOptions::BadUsageException(
                "empty - key at action arg "+std::to_string(i));

            mInput.sfiles.emplace(param, RunnerInput::FileStream{ "data", std::cin });
        }
        else // plain argument
        {
            mInput.params[param] = getNextValue(args, i);
        }
    }
}
