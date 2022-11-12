#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <string>
#include <chrono>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "Backend.hpp"
#include "Utilities.hpp"

namespace Andromeda {

/** Runs the API remotely over HTTP */
class HTTPRunner : public Backend::Runner
{
public:

    /** Exception indicating the HTTP library had an error */
    class Exception : public EndpointException { 
        /** @param error the library error code */
        public: Exception(httplib::Error error) : 
            EndpointException(httplib::to_string(error)) {} };

    /** Exception indicating that the connection to the server failed */
    class ConnectionException : public Exception {
        public: ConnectionException() : 
            Exception(httplib::Error::Connection) {} };

    /** HTTP config options */
    struct Options
    {
        /** maximum retries before throwing */
        unsigned long maxRetries { 12 };
        /** The time to wait between each retry */
        std::chrono::seconds retryTime { std::chrono::seconds(5) };
        /** The connection read/write timeout */
        std::chrono::seconds timeout { std::chrono::seconds(120) };
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

    /**
     * @param hostname to use with HTTP
     * @param baseURL URL of the endpoint
     * @param opts HTTP config options
     */
    HTTPRunner(const std::string& hostname, const std::string& baseURL, const Options& opts);

    virtual std::string RunAction(const Input& input) override;

    /** Allows automatic retry on HTTP failure */
    virtual void EnableRetry(bool enable = true) { this->canRetry = enable; }

    virtual bool RequiresSession() override { return true; }

private:

    Debug debug;

    Options options;

    std::string baseURL;
    httplib::Client httpClient;

    bool canRetry { false };
};

} // namespace Andromeda

#endif

