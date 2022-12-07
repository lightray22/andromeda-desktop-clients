#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <chrono>
#include <memory>
#include <string>

// TODO should not include this here - use PRIVATE cmake link
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "Backend.hpp"
#include "Utilities.hpp"

namespace Andromeda {

class HTTPRunnerFriend;

/** Runs the API remotely over HTTP */
class HTTPRunner : public Backend::Runner
{
public:

    /** Exception indicating the HTTP library had an error */
    class Exception : public EndpointException { 
        /** @param error the library error code */
        public: explicit Exception(httplib::Error error) : 
            EndpointException(httplib::to_string(error)) {} };

    /** Exception indicating that the connection to the server failed */
    class ConnectionException : public Exception {
        public: explicit ConnectionException() : 
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

    /**
     * @param protoHost (protocol://)hostname
     * @param baseURL baseURL of the endpoint
     * @param options HTTP config options
     */
    HTTPRunner(const std::string& protoHost, const std::string& baseURL, const Options& options);

    typedef std::pair<std::string, std::string> HostUrlPair;

    /**
     * Parse a URL for the constructor
     * @return std::pair<std::string> protoHost, baseURL pair
     */
    static HostUrlPair ParseURL(std::string fullURL);

    /** Returns the HTTP hostname (without proto://) */
    virtual std::string GetHostname() const override;

    /** Returns the proto://hostname string */
    virtual std::string GetProtoHost() const;

    /** Returns the base URL being used */
    virtual const std::string& GetBaseURL() const { return mBaseURL; }

    virtual std::string RunAction(const Input& input) override;

    /** Allows automatic retry on HTTP failure */
    virtual void EnableRetry(bool enable = true) { mCanRetry = enable; }

    /** Returns whether retry is enabled or disabled */
    virtual bool GetCanRetry() const { return mCanRetry; }

    virtual bool RequiresSession() override { return true; }

private:

    friend class HTTPRunnerFriend;

    /** Handles an HTTP redirect to a new location */
    virtual void HandleRedirect(const std::string& location);

    Debug mDebug;

    Options mOptions;

    std::string mProtoHost;
    std::string mBaseURL;

    std::unique_ptr<httplib::Client> mHttpClient;

    bool mCanRetry { false };
};

} // namespace Andromeda

#endif

