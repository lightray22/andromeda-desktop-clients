#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <chrono>
#include <memory>
#include <string>

// TODO should not include this here - use PRIVATE cmake link
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "BaseRunner.hpp"
#include "HTTPRunnerOptions.hpp"
#include "Utilities.hpp"

namespace Andromeda {

class HTTPRunnerFriend;
struct RunnerInput;

/** Runs the API remotely over HTTP */
class HTTPRunner : public BaseRunner
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

    /**
     * @param protoHost (protocol://)hostname
     * @param baseURL baseURL of the endpoint
     * @param options HTTP config options
     */
    HTTPRunner(const std::string& protoHost, const std::string& baseURL, const HTTPRunnerOptions& options);

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

    virtual std::string RunAction(const RunnerInput& input) override;

    /** Allows automatic retry on HTTP failure */
    virtual void EnableRetry(bool enable = true) { mCanRetry = enable; }

    /** Returns whether retry is enabled or disabled */
    virtual bool GetCanRetry() const { return mCanRetry; }

    virtual bool RequiresSession() override { return true; }

private:

    friend class HTTPRunnerFriend;

    /** Initializes the HTTP client */
    void InitializeClient(const std::string& protoHost);

    /** Handles an HTTP redirect to a new location */
    void HandleRedirect(const std::string& location);

    Debug mDebug;

    HTTPRunnerOptions mOptions;

    std::string mProtoHost;
    std::string mBaseURL;

    std::unique_ptr<httplib::Client> mHttpClient;

    bool mCanRetry { false };
};

} // namespace Andromeda

#endif

