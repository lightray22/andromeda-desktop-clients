
#include <sstream>

#include "RunnerOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/Utilities.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace Backend {

/*****************************************************/
std::string RunnerOptions::HelpText()
{
    std::ostringstream output;
    RunnerOptions optDefault;

    const auto defRetry(seconds(optDefault.retryTime).count());
    const auto defTimeout(seconds(optDefault.timeout).count());

    using std::endl;

    output << "Runner Advanced: [--req-timeout secs(" << defTimeout << ")] [--max-retries uint(" << optDefault.maxRetries << ")] [--retry-time secs(" << defRetry << ")] "
           << "[--stream-buffer-size bytes32(" << Utilities::bytesToString(optDefault.streamBufferSize) << ")]";

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
        try { maxRetries = stoul(value); }
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
        try { streamBufferSize = static_cast<size_t>(Utilities::stringToBytes(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!streamBufferSize) throw BaseOptions::BadValueException(option);
    }
    else return false; // not used

    return true; 
}

} // namespace Backend
} // namespace Andromeda
