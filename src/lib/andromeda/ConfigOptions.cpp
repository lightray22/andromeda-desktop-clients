
#include <sstream>

#include "ConfigOptions.hpp"
#include "BaseOptions.hpp"
#include "Utilities.hpp"

using namespace std::chrono;

namespace Andromeda {

/*****************************************************/
std::string ConfigOptions::HelpText()
{
    std::ostringstream output;
    ConfigOptions optDefault;

    const auto defRefresh(optDefault.refreshTime.count());
    const auto defReadAhead(optDefault.readAheadTime.count());

    using std::endl; output 
        << "Advanced:        [-q|--quiet] [-r|--read-only] [--dir-refresh secs(" << defRefresh << ")] [--cachemode none|memory|normal] [--backend-runners uint(" << optDefault.runnerPoolSize << ")]" << endl
        << "Data Advanced:   [--pagesize bytes32(" << Utilities::bytesToString(optDefault.pageSize) << ")] [--read-ahead ms(" << defReadAhead << ")]"
            << " [--read-max-cache-frac uint(" << optDefault.readMaxCacheFrac << ")] [--read-ahead-buffer pages(" << optDefault.readAheadBuffer << ")]";

    return output.str();
}

/*****************************************************/
bool ConfigOptions::AddFlag(const std::string& flag)
{
    if (flag == "q" || flag == "quiet")
        quiet = true;
    else if (flag == "r" || flag == "read-only")
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
    else if (option == "dir-refresh")
    {
        try { refreshTime = decltype(refreshTime)(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "backend-runners")
    {
        try { runnerPoolSize = decltype(runnerPoolSize)(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!runnerPoolSize) throw BaseOptions::BadValueException(option);
    }
    else if (option == "pagesize")
    {
        try { pageSize = static_cast<size_t>(Utilities::stringToBytes(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!pageSize) throw BaseOptions::BadValueException(option);
    }
    else if (option == "read-ahead")
    {
        try { readAheadTime = decltype(readAheadTime)(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "read-max-cache-frac")
    {
        try { readMaxCacheFrac = stoul(value); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!readMaxCacheFrac) throw BaseOptions::BadValueException(option);
    }
    else if (option == "read-ahead-buffer")
    {
        try { readAheadBuffer = stoul(value); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else return false; // not used

    return true; 
}

} // namespace Andromeda
