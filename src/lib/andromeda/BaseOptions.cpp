
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

    output << "Debugging:       [-d|--debug 0-" << static_cast<size_t>(Debug::Level::LAST) << "] [--debug-filter str1,str2+]";

    return output.str();
}

/*****************************************************/
size_t BaseOptions::ParseArgs(size_t argc, const char* const* argv, bool noerr)
{
    Flags flags; Options options;

    size_t i { 1 }; for (; i < argc; i++)
    {
        std::string key { argv[i] };
        if (key.empty())
            throw BadUsageException(
                "empty key at arg "+std::to_string(i));
        if (key[0] != '-') { 
            if (noerr) break; else throw BadUsageException(
                "expected key at arg "+std::to_string(i)); }

        key.erase(0, 1); // key++
        bool ext { (key[0] == '-') };
        if (ext) key.erase(0, 1); // --opt

        if (key.empty() || std::isspace(key[0]))
            throw BadUsageException(
                "empty key at arg "+std::to_string(i));
        
        if (key.find('=') != std::string::npos) // -x=3 or --x=3
        {
            const Utilities::StringPair pair { Utilities::split(key, "=") };
            options.emplace(pair.first, pair.second);
        }
        else if (!ext && key.size() > 1)
            options.emplace(key.substr(0, 1), key.substr(1)); // -x3
        else if (argc > i+1 && argv[i+1][0] != '-')
            options.emplace(key, argv[++i]); // -x 3, --x 3
        else flags.push_back(key); // -x, --x
    }

    for (const decltype(flags)::value_type& flag : flags) 
        if (!AddFlag(flag)) throw BadFlagException(flag);

    for (const decltype(options)::value_type& pair : options)
        if (!AddOption(pair.first, pair.second)) throw BadOptionException(pair.first);

    return i;
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
void BaseOptions::ParseConfig(const std::string& name)
{
    std::list<std::string> paths { 
        "/etc/andromeda", "/usr/local/etc/andromeda" };

    std::string home { Utilities::GetHomeDirectory() };
    if (!home.empty()) paths.push_back(home+"/.config/andromeda");

    paths.push_back("."); for (std::string path : paths)
    {
        path += "/"+name+".conf";
        if (std::filesystem::is_regular_file(path))
            ParseFile(path);
    }
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
    else return false; // not used

    return true;
}

/*****************************************************/
bool BaseOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "d" || option == "debug")
    {
        try { Debug::SetLevel(static_cast<Debug::Level>(stoul(value))); }
        catch (const std::logic_error& e) { 
            throw BadValueException(option); }
    }
    else if (option == "debug-filter")
    {
        for (const std::string& name : Utilities::explode(value,","))
            Debug::AddFilter(name);
    }
    else return false; // not used

    return true;
}

} // namespace Andromeda
