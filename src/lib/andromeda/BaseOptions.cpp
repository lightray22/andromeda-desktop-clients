
#include <cstring>
#include <fstream>
#include <sstream>

#include "BaseOptions.hpp"
#include "Debug.hpp"
#include "PlatformUtil.hpp"
#include "StringUtil.hpp"

namespace Andromeda {

/*****************************************************/
std::string BaseOptions::CoreBaseHelpText()
{
    std::ostringstream output;

    output << "(-h|--help | -V|--version)";

    return output.str();
}

/*****************************************************/
std::string BaseOptions::DetailBaseHelpText(const std::string& name)
{
    std::ostringstream output;
    using std::endl;

    output << "Config File:     [-c|--config-file path]" << endl
           << "Debugging:       [-d|--debug 0-" << static_cast<size_t>(Debug::Level::LAST) << "] [--debug-filter str1,str2+] [--debug-log path]" << endl << endl

           << "Any flag or option can also be listed in andromeda.conf";
    if (!name.empty()) output << " or andromeda-" << name << ".conf";
    output << " with one option=value per line.";

    return output.str();
}

/*****************************************************/
size_t BaseOptions::ParseArgs(size_t argc, const char* const* argv, bool stopmm)
{
    Flags flags; Options options;

    size_t argIdx { 1 }; for (; argIdx < argc; argIdx++)
    {
        std::string key { argv[argIdx] };
        if (key.empty())
            throw BadUsageException(
                "empty key at arg "+std::to_string(argIdx));
        if (key[0] != '-')
            throw BadUsageException(
                "expected key at arg "+std::to_string(argIdx));

        key.erase(0, 1); // key++
        const bool ext { (key[0] == '-') };
        if (ext) key.erase(0, 1); // --opt

        if (key.empty() || std::isspace(key[0]))
        {
            if (stopmm) { ++argIdx; break; }
            else throw BadUsageException(
                "empty key at arg "+std::to_string(argIdx));
        }
        
        if (key.find('=') != std::string::npos) // -x=3 or --x=3
        {
            const StringUtil::StringPair pair { StringUtil::split(key, "=") };
            options.emplace(pair.first, pair.second);
        }
        else if (!ext && key.size() > 1)
            options.emplace(key.substr(0, 1), key.substr(1)); // -x3
        else if (argc > argIdx+1 && argv[argIdx+1][0] != '-')
            options.emplace(key, argv[++argIdx]); // -x 3, --x 3
        else flags.push_back(key); // -x, --x
    }

    for (const decltype(flags)::value_type& flag : flags) 
        if (!AddFlag(flag)) throw BadFlagException(flag);

    for (const decltype(options)::value_type& pair : options)
        if (!AddOption(pair.first, pair.second)) throw BadOptionException(pair.first);

    return argIdx;
}

/*****************************************************/
void BaseOptions::ParseFile(const std::filesystem::path& path)
{
    Flags flags; Options options;

    std::ifstream file(path, std::ios::in | std::ios::binary);

    while (file.good())
    {
        std::string line; std::getline(file,line);

        // Windows-formatted files will have extra \r at the end of lines
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty() || line.at(0) == '#' || line.at(0) == ' ') continue;

        if (line.find('=') == std::string::npos) flags.push_back(line);
        else options.emplace(StringUtil::split(line, "="));
    }

    for (const decltype(flags)::value_type& flag : flags) 
        if (!AddFlag(flag)) throw BadFlagException(flag);

    for (const decltype(options)::value_type& pair : options)
        if (!AddOption(pair.first, pair.second)) throw BadOptionException(pair.first);
}

/*****************************************************/
void BaseOptions::ParseConfig(const std::string& prefix)
{
    std::list<std::string> paths { 
        "/etc/andromeda", "/usr/local/etc/andromeda" };

    const std::string home { PlatformUtil::GetHomeDirectory() };
    if (!home.empty()) paths.push_back(home+"/.config/andromeda");

    paths.emplace_back("."); for (std::string path : paths)
    {
        path += "/"+prefix+".conf";
        if (std::filesystem::is_regular_file(path))
            ParseFile(path);
    }
}

/*****************************************************/
void BaseOptions::ParseUrl(const std::string& url)
{
    Flags flags; Options options;

    const size_t sep(url.find('?'));

    if (sep != std::string::npos)
    {
        const std::string& substr(url.substr(sep+1));

        for (const std::string& param : StringUtil::explode(substr,"&"))
        {
            if (param.find('=') == std::string::npos) flags.push_back(param);
            else options.emplace(StringUtil::split(param, "="));
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
    else return false; // not used

    return true;
}

/*****************************************************/
bool BaseOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "c" || option == "config")
    {
        if (std::filesystem::is_regular_file(value))
            ParseFile(value);
        else throw BadValueException(option);
    }
    if (option == "d" || option == "debug")
    {
        try { Debug::SetLevel(static_cast<Debug::Level>(stoul(value))); }
        catch (const std::logic_error& e) { 
            throw BadValueException(option); }
    }
    else if (option == "debug-filter")
    {
        Debug::SetFilters(value);
    }
    else if (option == "debug-log")
    {
        Debug::AddLogFile(value); // path
    }
    else return false; // not used

    return true;
}

} // namespace Andromeda
