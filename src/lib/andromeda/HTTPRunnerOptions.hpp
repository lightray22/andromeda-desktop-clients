#ifndef LIBA2_HTTPRUNNEROPTIONS_H_
#define LIBA2_HTTPRUNNEROPTIONS_H_

#include <chrono>
#include <string>

namespace Andromeda {

/** HTTP config options */
struct HTTPRunnerOptions
{
    /** maximum retries before throwing */
    unsigned long maxRetries { 12 };
    /** The time to wait between each retry */
    std::chrono::seconds retryTime { std::chrono::seconds(5) };
    /** The connection read/write timeout */
    std::chrono::seconds timeout { std::chrono::seconds(120) };
    /** Whether or not redirects are allowed */
    bool followRedirects { true };
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

} // namespace Andromeda

#endif // LIBA2_HTTPRUNNEROPTIONS_H_
