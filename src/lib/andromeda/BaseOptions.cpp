
#include <cstring>
#include <fstream>
#include <sstream>

#include "BaseOptions.hpp"
#include "Debug.hpp"
#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
std::string BaseOptions::CoreBaseHelpText()
{
    std::ostringstream output;

    output << "(-h|--help | -V|--version)";

    return output.str();
}

/*****************************************************/
std::string BaseOptions::OtherBaseHelpText()
{
    std::ostringstream output;

    output << "Debugging:       [-d|--debug [0-" << sizeof(Debug::Level) << "]]";

    return output.str();
}

/*****************************************************/
void BaseOptions::ParseArgs(int argc, const char* const* argv)
{
    Flags flags; Options options;

    for (int i { 1 }; i < argc; i++)
    {
        if (strlen(argv[i]) < 2 || argv[i][0] != '-')
            throw BadUsageException();

        const char* flag { argv[i]+1 };
        bool ext { (flag[0] == '-') };
        if (ext) flag++; // --opt

        if (!ext && strlen(flag) > 1)
            options.emplace(std::string(1,flag[0]), flag+1); // -x3

        else if (argc-1 > i && argv[i+1][0] != '-')
            options.emplace(flag, argv[++i]); // -x 3

        else flags.push_back(flag); // -x
    }

    for (const decltype(flags)::value_type& flag : flags) 
        if (!AddFlag(flag)) throw BadFlagException(flag);

    for (const decltype(options)::value_type& pair : options)
        if (!AddOption(pair.first, pair.second)) throw BadOptionException(pair.first);
}

/*****************************************************/
void BaseOptions::ParseFile(const std::filesystem::path& path)
{
    Flags flags; Options options;

    std::ifstream file(path, std::ios::in | std::ios::binary);

    while (file.good())
    {
        std::string line; std::getline(file,line);

        if (!line.size() || line.at(0) == '#' || line.at(0) == ' ') continue;

        if (line.find("=") == std::string::npos) flags.push_back(line);
        else options.emplace(Utilities::split(line, "="));
    }

    for (const decltype(flags)::value_type& flag : flags) 
        if (!AddFlag(flag)) throw BadFlagException(flag);

    for (const decltype(options)::value_type& pair : options)
        if (!AddOption(pair.first, pair.second)) throw BadOptionException(pair.first);
}

/*****************************************************/
void BaseOptions::ParseUrl(const std::string& url)
{
    Flags flags; Options options;

    const size_t sep(url.find("?"));

    if (sep != std::string::npos)
    {
        const std::string& substr(url.substr(sep+1));

        for (const std::string& param : Utilities::explode(substr,"&"))
        {
            if (param.find("=") == std::string::npos) flags.push_back(param);
            else options.emplace(Utilities::split(param, "="));
        }
    }

    for (const decltype(flags)::value_type& flag : flags) 
        TryAddUrlFlag(flag);

    for (const decltype(options)::value_type& pair : options)
        TryAddUrlOption(pair.first, pair.second);
}

/*****************************************************/
bool BaseOptions::AddFlag(const std::string& flag)
{
    if (flag == "h" || flag == "help")
        throw ShowHelpException();
    else if (flag == "V" || flag == "version")
        throw ShowVersionException();
    else if (flag == "d" || flag == "debug")
        mDebugLevel = Debug::Level::ERRORS;
    else return false; // not used

    return true;
}

/*****************************************************/
bool BaseOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "d" || option == "debug")
    {
        try { mDebugLevel = static_cast<Debug::Level>(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BadValueException(option); }
    }
    else return false; // not used

    return true;
}

} // namespace Andromeda
