
#include <sstream>

#include "CacheOptions.hpp"
#include "andromeda/BaseOptions.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
std::string CacheOptions::HelpText()
{
    std::ostringstream output;
    CacheOptions cfgDefault;

    const auto defDirty(milliseconds(cfgDefault.maxDirtyTime).count());

    // TODO add utility functions to convert 268435456 <-> 256M + unit test
    output << "Cache Advanced:  [--max-dirty ms(" << defDirty << ")] [--memory-limit bytes64(" << cfgDefault.memoryLimit << ")] [--evict-frac uint(" << cfgDefault.evictSizeFrac << ")]";

    return output.str();
}

/*****************************************************/
bool CacheOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "max-dirty")
    {
        try { maxDirtyTime = decltype(maxDirtyTime)(stoul(value)); }
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
    }
    else if (option == "memory-limit")
    {
        try { memoryLimit = stoull(value); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "evict-frac")
    {
        try { evictSizeFrac = stoul(value); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!evictSizeFrac) throw BaseOptions::BadValueException(option);
    }
    else return false; // not used

    return true; 
}

} // namespace Filedata
} // namespace Filesystme
} // namespace Andromeda
