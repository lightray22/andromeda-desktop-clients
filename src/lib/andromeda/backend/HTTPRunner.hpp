#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <chrono>
#include <memory>
#include <string>

// TODO should not include this here - use PRIVATE cmake link
// probably need just an interface here then a separate HTTPRunnerImpl
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#if WIN32 // httplib adds windows.h
// thanks for nothing, Windows >:(
#undef CreateFile
#undef DeleteFile
#undef MoveFile
#endif // WIN32

#include "BaseRunner.hpp"
#include "HTTPOptions.hpp"
#include "RunnerOptions.hpp"
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

    /** Exception indicating that the request was redirected */
    class RedirectException : public EndpointException { public:
        RedirectException() : EndpointException("Redirected") {}
        explicit RedirectException(const std::string& location) :
            EndpointException("30X Redirected: "+location) {}; };

    /** Exception indicating the input request size is too big */
    class InputSizeException : public EndpointException { public:
        InputSizeException() : EndpointException("413 Request Too Large") {} };

    /**
     * @param protoHost (protocol://)hostname
     * @param baseURL baseURL of the endpoint
     * @param userAgent name of the program running
     * @param runnerOptions base runner config options
     * @param httpOptions HTTP config options
     */
    HTTPRunner(const std::string& protoHost, const std::string& baseURL, const std::string& userAgent,
        const RunnerOptions& runnerOptions, const HTTPOptions& httpOptions);

    virtual std::unique_ptr<BaseRunner> Clone() const override;

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

    inline virtual std::string RunAction_Read(const RunnerInput& input) override { 
        bool isJson; return RunAction_Read(input, isJson); };

    inline virtual std::string RunAction_Write(const RunnerInput& input) override { 
        bool isJson; return RunAction_Write(input, isJson); };

    virtual std::string RunAction_FilesIn(const RunnerInput_FilesIn& input) override { 
        bool isJson; return RunAction_FilesIn(input, isJson); };
    
    virtual std::string RunAction_StreamIn(const RunnerInput_StreamIn& input) override { 
        bool isJson; return RunAction_StreamIn(input, isJson); };
    
    virtual void RunAction_StreamOut(const RunnerInput_StreamOut& input) override { 
        bool isJson; RunAction_StreamOut(input, isJson); };

    /** @param[out] isJson ref set to whether response is json */
    virtual std::string RunAction_Read(const RunnerInput& input, bool& isJson);

    /** @param[out] isJson ref set to whether response is json */
    virtual std::string RunAction_Write(const RunnerInput& input, bool& isJson);

    /** @param[out] isJson ref set to whether response is json */
    virtual std::string RunAction_FilesIn(const RunnerInput_FilesIn& input, bool& isJson);

    /** @param[out] isJson ref set to whether response is json */
    virtual std::string RunAction_StreamIn(const RunnerInput_StreamIn& input, bool& isJson);

    /** @param[out] isJson ref set to whether response is json (before data starts) */
    virtual void RunAction_StreamOut(const RunnerInput_StreamOut& input, bool& isJson);

    virtual bool RequiresSession() const override { return true; }

private:

    friend class HTTPRunnerFriend;

    /** Initializes the HTTP client */
    void InitializeClient(const std::string& protoHost);

    /** 
     * Gets request info for the given input, adding plainParams to the URL
     * @param dataParams if true, add dataParams to the headers
     * @return the URL to use for the request
     */
    std::string SetupRequest(const RunnerInput& input, httplib::Headers& headers, bool dataParams = true);

    /** Add dataParams from the given input to a post body */
    void AddDataParams(const RunnerInput& input, httplib::MultipartFormDataItems& params);

    /** Add filesIn params from the given input to a post body */
    void AddFileParams(const RunnerInput_FilesIn& input, httplib::MultipartFormDataItems& params);

    /**
     * Performs a series of HTTP request attempts, caller must call HandleResponse
     * @param[in] getResult Function that provides an httplib result
     * @param[out] canRetry ref set to where retry is allowed
     * @param[in] doRetry ref to bool set by manual call of HandleResponse
     */
    void DoRequests(std::function<httplib::Result()> getResult, bool& canRetry, const bool& doRetry);

    /**
     * Performs a series of HTTP request attempts, calling HandleResponse
     * @param[in] getResult Function that provides an httplib result
     * @param[out] isJson ref set to whether response is json
     * @return std::string the body of the response
     */
    std::string DoRequestsFull(std::function<httplib::Result()> getResult, bool& isJson);

    /**
     * Handles an httplib non-response
     * @param result httplib result object
     * @param retry true if retry is allowed (wait), else throw
     * @param attempt current attempt # (for debug print)
     * @throws LibraryException if not retry
     */
    void HandleNonResponse(httplib::Result& result, const bool retry, const size_t attempt);

    /**
     * Handles an httplib response
     * @param[in] response httplib response object
     * @param[out] isJson ref set to whether response is json
     * @param[in] canRetry true if a retry is allowed (else fail)
     * @param[out] doRetry ref set to whether the request needs retry
     * @return std::string the body if not doRetry
     * @throws EndpointException if a non-retry error
     */
    std::string HandleResponse(const httplib::Response& response, bool& isJson, const bool& canRetry, bool& doRetry);

    /** Handles an HTTP redirect to a new location */
    void RegisterRedirect(const std::string& location);

    /** Returns a redirect exception with the response */
    RedirectException GetRedirectException(const httplib::Response& response);

    Debug mDebug;

    std::string mProtoHost;
    std::string mBaseURL;
    std::string mUserAgent;

    const RunnerOptions mBaseOptions;
    const HTTPOptions mHttpOptions;

    std::unique_ptr<httplib::Client> mHttpClient;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_HTTPRUNNER_H_
