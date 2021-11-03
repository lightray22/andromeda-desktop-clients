#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <string>
#include <chrono>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "Backend.hpp"
#include "Utilities.hpp"

/** Runs the API remotely over HTTP */
class HTTPRunner : public Backend::Runner
{
public:

    /** Exception indicating the HTTP library had an error */
    class Exception : public Backend::Exception { 
        public: Exception(httplib::Error error) : 
            Backend::Exception(httplib::to_string(error)) {} };

    /** Exception indicating that the connection to the server failed */
    class ConnectionException : public Exception {
        public: ConnectionException() : 
            Exception(httplib::Error::Connection) {} };

    /** HTTP config options */
    struct Options
    {
        size_t maxRetries = 12;
        std::chrono::seconds retryTime = std::chrono::seconds(5);
        std::chrono::seconds timeout = std::chrono::seconds(120);

        std::string username;
        std::string password;
        std::string proxyHost;
        short proxyPort = 443;
        std::string proxyUsername;
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

private:

    Debug debug;

    Options options;

    std::string baseURL;
    httplib::Client httpClient;

    bool canRetry = false;
};

#endif