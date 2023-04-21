
#include <sstream>

#include "HTTPOptions.hpp"
#include "andromeda/BaseOptions.hpp"
#include "andromeda/Utilities.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace Backend {

/*****************************************************/
std::string HTTPOptions::HelpText()
{
    std::ostringstream output;
    HTTPOptions optDefault;

    const auto defRetry(seconds(optDefault.retryTime).count());
    const auto defTimeout(seconds(optDefault.timeout).count());

    using std::endl;

    output 
        << "HTTP Options:    [--http-user str --http-pass str] [--hproxy-host host [--hproxy-port uint16] [--hproxy-user str --hproxy-pass str]] [--no-tls-verify]" << endl
        << "HTTP Advanced:   [--no-http-redirect] [--http-timeout secs(" << defTimeout << ")] [--max-retries uint(" << optDefault.maxRetries << ")] [--retry-time secs(" << defRetry << ")] [--stream-buffer-size bytes32(" << optDefault.streamBufferSize << ")]";

    return output.str();
}

/*****************************************************/
bool HTTPOptions::AddFlag(const std::string& flag)
{
    if (flag == "no-tls-verify")
        tlsCertVerify = false;
    else if (flag == "no-http-redirect")
        followRedirects = false;
    else return false; // not used

    return true;
}

/*****************************************************/
bool HTTPOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "http-user")
        username = value;
    else if (option == "http-pass")
        password = value;
    else if (option == "hproxy-host")
        proxyHost = value;
    else if (option == "hproxy-port")
    {
        try { proxyPort = static_cast<decltype(proxyPort)>(stoul(value)); }
        catch (const std::logic_error& e) {
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "hproxy-user")
        proxyUsername = value;
    else if (option == "hproxy-pass")
        proxyPassword = value;
    else if (option == "http-timeout")
    {
        try { timeout = seconds(stoul(value)); }
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
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
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
    }
    else if (option == "stream-buffer-size")
    {
        try { streamBufferSize = stoul(value); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }

        if (!streamBufferSize) throw BaseOptions::BadValueException(option);
    }
    else return false; // not used

    return true; 
}

} // namespace Backend
} // namespace Andromeda
