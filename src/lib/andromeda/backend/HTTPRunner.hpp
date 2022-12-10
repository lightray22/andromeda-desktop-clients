#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <chrono>
#include <memory>
#include <string>

// TODO should not include this here - use PRIVATE cmake link
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "BaseRunner.hpp"
#include "HTTPOptions.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Backend {

class HTTPRunnerFriend;
struct RunnerInput;

/** Runs the API remotely over HTTP */
class HTTPRunner : public BaseRunner
{
public:

    /** Exception indicating the HTTP library had an error */
    class LibraryException : public EndpointException { 
        /** @param error the library error code */
        public: explicit LibraryException(httplib::Error error) : 
            EndpointException(httplib::to_string(error)) {} };

    /** Exception indicating that the connection to the server failed */
    class ConnectionException : public LibraryException {
        public: explicit ConnectionException() : 
            LibraryException(httplib::Error::Connection) {} };

    /**
     * @param protoHost (protocol://)hostname
     * @param baseURL baseURL of the endpoint
     * @param options HTTP config options
     */
    HTTPRunner(const std::string& protoHost, const std::string& baseURL, const HTTPOptions& options);

    typedef std::pair<std::string, std::string> HostUrlPair;

    /**
     * Parse a URL for the constructor
     * @return std::pair<std::string> protoHost, baseURL pair
     */
    static HostUrlPair ParseURL(std::string fullURL);

    /** Returns the HTTP hostname (without proto://) */
    virtual std::string GetHostname() const override;

    /** Returns the proto://hostname string */
    virtual std::string GetProtoHost() const final;

    /** Returns the base URL being used */
    virtual const std::string& GetBaseURL() const final { return mBaseURL; }

    inline virtual std::string RunAction(const RunnerInput& input) override { 
        bool isJson; return RunAction(input,isJson); };

    /** @param isJson if the response is JSON, set to true */
    virtual std::string RunAction(const RunnerInput& input, bool& isJson);

    /** Allows automatic retry on HTTP failure */
    virtual void EnableRetry(bool enable = true) final { mCanRetry = enable; }

    /** Returns whether retry is enabled or disabled */
    virtual bool GetCanRetry() const final { return mCanRetry; }

    virtual bool RequiresSession() override { return true; }

private:

    friend class HTTPRunnerFriend;

    /** Initializes the HTTP client */
    void InitializeClient(const std::string& protoHost);

    /** Returns an httplib function that reads in to buf */
    httplib::ContentProviderWithoutLength GetStreamFunc(
        char* const& buf, const size_t& bufSize, std::istream& in);

    /** Throws a redirect exception with the repsonse headers */
    void RedirectException(const httplib::Headers& headers);

    /** Handles an HTTP redirect to a new location */
    void HandleRedirect(const std::string& location);

    Debug mDebug;

    HTTPOptions mOptions;

    std::string mProtoHost;
    std::string mBaseURL;

    std::unique_ptr<httplib::Client> mHttpClient;

    bool mCanRetry { false };
};

} // namespace Backend
} // namespace Andromeda

#endif

