
#include <sstream>

#include "CacheOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/Utilities.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
std::string CacheOptions::HelpText()
{
    std::ostringstream output;
    CacheOptions optDefault;

    const auto defDirty(milliseconds(optDefault.maxDirtyTime).count());

    output << "Cache Advanced:  [--max-dirty ms(" << defDirty << ")] [--memory-limit bytes64(" << optDefault.memoryLimit << ")] [--evict-frac uint(" << optDefault.evictSizeFrac << ")]";

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
        try { memoryLimit = Utilities::stringToBytes(value); }
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
