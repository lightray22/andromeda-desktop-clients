
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "HTTPRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/base64.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& protoHost, const std::string& baseURL, const HTTPOptions& options) : 
    mDebug("HTTPRunner",this), mOptions(options), mProtoHost(protoHost), mBaseURL(baseURL)
{
    if (!Utilities::startsWith(mBaseURL,"/")) mBaseURL.insert(0, "/");

    MDBG_INFO("(protoHost:" << mProtoHost << " baseURL:" << mBaseURL << ")");

    InitializeClient(mProtoHost);
}

/*****************************************************/
void HTTPRunner::InitializeClient(const std::string& protoHost)
{
    mHttpClient = std::make_unique<httplib::Client>(protoHost);

    if (mOptions.followRedirects)
        mHttpClient->set_follow_location(true);

    mHttpClient->set_keep_alive(true);
    mHttpClient->set_read_timeout(mOptions.timeout);
    mHttpClient->set_write_timeout(mOptions.timeout);

    if (!mOptions.username.empty())
    {
        mHttpClient->set_basic_auth(
            mOptions.username.c_str(), mOptions.password.c_str());
    }

    if (!mOptions.proxyHost.empty())
    {
        mHttpClient->set_proxy(
            mOptions.proxyHost.c_str(), mOptions.proxyPort);
    }

    if (!mOptions.proxyUsername.empty())
    {
        mHttpClient->set_proxy_basic_auth(
            mOptions.proxyUsername.c_str(), mOptions.proxyPassword.c_str());
    }
}

/*****************************************************/
HTTPRunner::HostUrlPair HTTPRunner::ParseURL(std::string fullURL)
{
    bool hasProto = fullURL.find("://") != std::string::npos;
    Utilities::StringPair pair { Utilities::split(fullURL, "/", hasProto ? 2 : 0) };

    if (!Utilities::startsWith(pair.second,"/")) 
        pair.second.insert(0, "/");

    return pair;
}

/*****************************************************/
std::string HTTPRunner::GetHostname() const
{
    Utilities::StringPair pair { Utilities::split(mProtoHost, "://") };
    return pair.second.empty() ? pair.first : pair.second; 
}

/*****************************************************/
std::string HTTPRunner::GetProtoHost() const
{
    return (mProtoHost.find("://") != std::string::npos)
        ? mProtoHost : ("http://"+mProtoHost);
}

/*****************************************************/
std::string HTTPRunner::SetupRequest(const RunnerInput& input, httplib::Headers& headers)
{
    // set up params as base64-encoded X-Andromeda headers
    for (const decltype(input.params)::value_type& it : input.params)
    {
        std::string key { it.first };
        std::replace(key.begin(), key.end(), '_', '-');
        headers.emplace("X-Andromeda-"+key, base64::encode(it.second));
    }

    // set up the URL parameters and query string
    httplib::Params urlParams {{"api",""},{"app",input.app},{"action",input.action}};

    return std::string(mBaseURL + (mBaseURL.find("?") != std::string::npos ? "&" : "?") + 
        httplib::detail::params_to_query_str(urlParams));
}

/*****************************************************/
std::string HTTPRunner::SetupRequest(const RunnerInput& input, httplib::MultipartFormDataItems& postParams)
{
    // set up params as POST body inputs
    for (const decltype(input.params)::value_type& it : input.params)
        postParams.push_back({it.first, it.second, {}, {}});

    // set up the URL parameters and query string
    httplib::Params urlParams {{"api",""},{"app",input.app},{"action",input.action}};

    return std::string(mBaseURL + (mBaseURL.find("?") != std::string::npos ? "&" : "?") + 
        httplib::detail::params_to_query_str(urlParams));
}

/*****************************************************/
void HTTPRunner::DoRequests(std::function<httplib::Result()> getResult, bool& canRetry, const bool& doRetry)
{
    // do the request some number of times until success
    for (decltype(mOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        canRetry = (mCanRetry && attempt <= mOptions.maxRetries);
        httplib::Result result { getResult() };
        if (result != nullptr && !doRetry) return; // break
        else HandleNonResponse(result, canRetry, attempt);
    }
}

/*****************************************************/
std::string HTTPRunner::DoRequestsFull(std::function<httplib::Result()> getResult, bool& isJson)
{
    // do the request some number of times until success
    for (decltype(mOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        bool canRetry = (mCanRetry && attempt <= mOptions.maxRetries);
        httplib::Result result { getResult() };
        if (result != nullptr)
        {
            bool doRetry = false; std::string retval { 
                HandleResponse(*result, isJson, canRetry, doRetry) }; 
            if (!doRetry) return retval; // break
        }

        HandleNonResponse(result, canRetry, attempt);
    }
}

/*****************************************************/
void HTTPRunner::HandleNonResponse(httplib::Result& result, const bool retry, const size_t attempt)
{
    MDBG_INFO("(retry:" << retry << ")");

    if (retry)
    {
        static const std::string fname(__func__);
        mDebug.Error([&](std::ostream& str)
        { 
            str << fname << "... ";
            
            if (result != nullptr) str << "HTTP " << result->status;
            else str << httplib::to_human_string(result.error());

            str << " error, attempt " << attempt+1 << " of " << mOptions.maxRetries+1;
        });

        std::this_thread::sleep_for(mOptions.retryTime);
    }
    else if (result.error() == httplib::Error::Connection)
            throw ConnectionException();
    else throw LibraryException(result.error());
}

/*****************************************************/
std::string HTTPRunner::HandleResponse(const httplib::Response& response, bool& isJson, const bool& canRetry, bool& doRetry)
{
    MDBG_INFO("() HTTP:" << response.status);

    doRetry = (canRetry && response.status == 503); 
    if (doRetry) return ""; // early return

    // if redirected, should remember it for next time
    if (mOptions.followRedirects && !response.location.empty()) 
        RegisterRedirect(response.location);

    switch (response.status)
    {
        case 200: // OK
        {
            isJson = (response.has_header("Content-type") && 
                response.get_header_value("Content-type") == "application/json");
            return std::move(response.body);
        }
        
        case 301: case 302: // HTTP redirect
            throw GetRedirectException(response); break;
        
        case 403: throw EndpointException("Access Denied");
        case 404: throw EndpointException("Not Found");
        default:  throw EndpointException(response.status);
    }
}

/*****************************************************/
std::string HTTPRunner::RunAction(const RunnerInput& input, bool& isJson)
{
    MDBG_INFO("()");

    httplib::Headers headers; std::string url(SetupRequest(input, headers));

    return DoRequestsFull([&](){ return mHttpClient->Get(url.c_str(), headers); }, isJson);
}

/*****************************************************/
std::string HTTPRunner::RunAction(const RunnerInput_FilesIn& input, bool& isJson)
{
    MDBG_INFO("(FilesIn)");

    // set up the POST body as multipart files
    httplib::MultipartFormDataItems postParams;
    std::string url(SetupRequest(input, postParams));

    for (const decltype(input.files)::value_type& it : input.files)
        postParams.push_back({it.first, it.second.data, it.second.name, {}});

    return DoRequestsFull([&](){ return mHttpClient->Post(url.c_str(), postParams); }, isJson);
}

/*****************************************************/
std::string HTTPRunner::RunAction(const RunnerInput_StreamIn& input, bool& isJson)
{
    MDBG_INFO("(StreamIn)");

    httplib::MultipartFormDataItems postParams;
    std::string url(SetupRequest(input, postParams));

    for (const decltype(input.files)::value_type& it : input.files)
        postParams.push_back({it.first, it.second.data, it.second.name, {}});

    // set up the POST body as files being done via a chunked stream
    httplib::MultipartFormDataProviderItems streamParams;

    const size_t bufSize { mOptions.streamBufferSize }; // copy
    std::unique_ptr<char[]> streamBuffer { !input.fstreams.empty()
        ? std::make_unique<char[]>(bufSize) : nullptr };

    for (const decltype(input.fstreams)::value_type& it : input.fstreams)
    {
        // callback that reads from the provided file stream into our buffer then httplib's buffer
        httplib::ContentProviderWithoutLength sfunc { [&](size_t offset, httplib::DataSink& sink)->bool
        {
            size_t read; bool hasMore { it.second.streamer(offset, streamBuffer.get(), bufSize, read) };
            sink.os.write(streamBuffer.get(), static_cast<std::streamsize>(read));
            if (!hasMore) sink.done(); 
            return true;
        }};

        streamParams.push_back({it.first, sfunc, it.second.name, {}});
    }
    
    return DoRequestsFull([&](){ return mHttpClient->Post(url.c_str(), { }, postParams, streamParams); }, isJson);
}

/*****************************************************/
void HTTPRunner::RunAction(const RunnerInput_StreamOut& input, bool& isJson)
{
    MDBG_INFO("(StreamOut)");

    // httplib only supports ContentReceiver with Get() so use headers instead of MultiPart
    httplib::Headers headers; std::string url(SetupRequest(input, headers));

    bool canRetry, doRetry; size_t offset;

    // Separate ResponseHandler callback as we need to check the response before streaming
    httplib::ResponseHandler respFunc { [&](const httplib::Response& response)->bool {
        HandleResponse(response, isJson, canRetry, doRetry); 
        offset = 0; return true;
    }};

    httplib::ContentReceiver recvFunc { [&](const char* data, size_t length)->bool {
        input.streamer(offset, data, length); 
        offset += length; return true;
    }};

    // First we call DoRequests which will set canRetry (by ref), then runs the httplib Get(). 
    // httplib then calls our ResponseHandler, which checks the response and canRetry, then sets doRetry if needed.  
    // httplib then returns to DoRequests which checks the doRetry and starts over if set.

    DoRequests([&](){ return mHttpClient->Get(url.c_str(), headers, respFunc, recvFunc); }, canRetry, doRetry);
}

/*****************************************************/
void HTTPRunner::RegisterRedirect(const std::string& location)
{
    MDBG_INFO("(location:" << location << ")");

    HTTPRunner::HostUrlPair newPair { HTTPRunner::ParseURL(location) };

    const size_t paramsPos { newPair.second.find("?") };
    if (paramsPos != std::string::npos) // remove URL params
        newPair.second.erase(paramsPos);
    
    if (newPair.first != GetProtoHost())
    {
        MDBG_INFO("... new protoHost:" << newPair.first);
        mProtoHost = newPair.first;
        InitializeClient(mProtoHost);
    }

    if (newPair.second != mBaseURL)
    {
        MDBG_INFO("... new baseURL:" << newPair.second);
        mBaseURL = newPair.second;
    }
}

/*****************************************************/
HTTPRunner::RedirectException HTTPRunner::GetRedirectException(const httplib::Response& response)
{
    if (response.has_header("Location"))
    {
        std::string location { response.get_header_value("Location") };
        
        const size_t paramsPos { location.find("?") };
        if (paramsPos != std::string::npos) // remove URL params
            location.erase(paramsPos);

        return RedirectException(location);
    }
    else return RedirectException();
}

} // namespace Backend
} // namespace Andromeda
