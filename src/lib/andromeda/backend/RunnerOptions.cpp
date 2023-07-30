
#include <sstream>

#include "RunnerOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/StringUtil.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
std::string RunnerOptions::HelpText()
{
    std::ostringstream output;
    const RunnerOptions optDefault;

    const auto defRetry(seconds(optDefault.retryTime).count());
    const auto defTimeout(seconds(optDefault.timeout).count());
    const size_t stBits { sizeof(size_t)*8 };

    using std::endl;

    output << "Runner Advanced: [--req-timeout secs(" << defTimeout << ")] [--max-retries uint32(" << optDefault.maxRetries << ")] [--retry-time secs(" << defRetry << ")] "
           << "[--stream-buffer-size bytes"<<stBits<<"(" << StringUtil::bytesToString(optDefault.streamBufferSize) << ")]";

    return output.str();
}

/*****************************************************/
bool RunnerOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "req-timeout")
    {
        try { timeout = seconds(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "max-retries")
    {
        try { maxRetries = static_cast<decltype(maxRetries)>(stoul(value)); }
        catch (const std::logic_error& e) {
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "retry-time")
    {
        try { retryTime = seconds(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "stream-buffer-size")
    {
        try { streamBufferSize = static_cast<size_t>(StringUtil::stringToBytes(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!streamBufferSize) throw BaseOptions::BadValueException(option);
    }
    else return false; // not used

    return true; 
}

} // namespace Backend
} // namespace Andromeda
