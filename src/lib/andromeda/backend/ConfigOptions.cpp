
#include <sstream>

#include "ConfigOptions.hpp"
#include "andromeda/BaseOptions.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace Backend {

/*****************************************************/
std::string ConfigOptions::HelpText()
{
    std::ostringstream output;

    ConfigOptions cfgDefault;

    const auto defRefresh(seconds(cfgDefault.refreshTime).count());

    output << "Advanced:        [-r|--read-only] [--pagesize bytes(" << cfgDefault.pageSize << ")] [--refresh secs(" << defRefresh << ")] [--cachemode none|memory|normal]";

    return output.str();
}
/*****************************************************/
bool ConfigOptions::AddFlag(const std::string& flag)
{
    if (flag == "r" || flag == "read-only")
        readOnly = true;
    else return false; // not used

    return true;
}

/*****************************************************/
bool ConfigOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "cachemode")
    {
             if (value == "none")   cacheType = ConfigOptions::CacheType::NONE;
        else if (value == "memory") cacheType = ConfigOptions::CacheType::MEMORY;
        else if (value == "normal") cacheType = ConfigOptions::CacheType::NORMAL;
        else throw BaseOptions::BadValueException(option);
    }
    else if (option == "pagesize")
    {
        try { pageSize = stoul(value); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!pageSize) throw BaseOptions::BadValueException(option);
    }
    else if (option == "refresh")
    {
        try { refreshTime = seconds(stoul(value)); }
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
    }
    else return false; // not used

    return true; 
}

} // namespace Backend
} // namespace Andromeda
