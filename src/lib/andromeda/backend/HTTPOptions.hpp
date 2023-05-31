#ifndef LIBA2_HTTPOPTIONS_H_
#define LIBA2_HTTPOPTIONS_H_

#include <chrono>
#include <string>

namespace Andromeda {
namespace Backend {

/** HTTP config options */
struct HTTPOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Adds the given argument, returning true iff it was used */
    bool AddFlag(const std::string& flag);

    /** Adds the given option/value, returning true iff it was used */
    bool AddOption(const std::string& option, const std::string& value);

    /** Whether or not redirects are allowed */
    bool followRedirects { true };
    /** Whether or not TLS cert verification is required */
    bool tlsCertVerify { true };
    /** HTTP basic-auth username */
    std::string username;
    /** HTTP basic-auth password */
    std::string password;
    /** HTTP proxy server hostname */
    std::string proxyHost;
    /** HTTP proxy server port */
    uint16_t proxyPort { 443 };
    /** HTTP proxy server basic-auth username */
    std::string proxyUsername;
    /** HTTP proxy server basic-auth password */
    std::string proxyPassword;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_HTTPOPTIONS_H_
