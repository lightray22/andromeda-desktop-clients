
#include <sstream>

#include "HTTPOptions.hpp"
#include "andromeda/BaseOptions.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace Backend {

/*****************************************************/
std::string HTTPOptions::HelpText()
{
    std::ostringstream output;

    output << "HTTP Options:    [--http-user str --http-pass str] [--hproxy-host host [--hproxy-port uint16] [--hproxy-user str --hproxy-pass str]]"
           << " [--no-tls-verify] [--no-http-redirect]";
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
    else return false; // not used

    return true; 
}

} // namespace Backend
} // namespace Andromeda
