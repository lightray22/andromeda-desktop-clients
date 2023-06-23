
#include <sstream>

#include "CacheOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Filedata {

/*****************************************************/
std::string CacheOptions::HelpText()
{
    std::ostringstream output;
    const CacheOptions optDefault;

    const auto defDirty(milliseconds(optDefault.maxDirtyTime).count());
    const size_t stBits { sizeof(size_t)*8 };

    output << "Cache Advanced:  [--max-dirty ms(" << defDirty << ")]"
        << " [--memory-limit bytes"<<stBits<<"(" << Utilities::bytesToString(optDefault.memoryLimit) << ")]"
        << " [--evict-frac uint32(" << optDefault.evictSizeFrac << ")]";

    return output.str();
}

/*****************************************************/
bool CacheOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "max-dirty")
    {
        try { maxDirtyTime = static_cast<decltype(maxDirtyTime)>(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "memory-limit")
    {
        try { memoryLimit = static_cast<size_t>(Utilities::stringToBytes(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "evict-frac")
    {
        try { evictSizeFrac = static_cast<decltype(evictSizeFrac)>(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!evictSizeFrac) throw BaseOptions::BadValueException(option);
    }
    else return false; // not used

    return true; 
}

} // namespace Filedata
} // namespace Filesystem
} // namespace Andromeda
