
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "HTTPRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/base64.hpp"
#include "andromeda/StringUtil.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& fullURL, const std::string& userAgent,
    const RunnerOptions& runnerOptions, const HTTPOptions& httpOptions) : 
    mDebug(__func__,this), mUserAgent(userAgent),
    mBaseOptions(runnerOptions), mHttpOptions(httpOptions),
    // allocate the stream buffer once so we don't alloc/free memory repeatedly
    mStreamBuffer(mBaseOptions.streamBufferSize) // not thread safe between requests!
{
    const HostUrlPair urlPair { ParseURL(fullURL) };
    mProtoHost = urlPair.first;
    mBaseURL = urlPair.second;

    MDBG_INFO("(url:" << fullURL << ") protoHost:" << mProtoHost << " baseURL:" << mBaseURL);

    InitializeClient(mProtoHost);
}

/*****************************************************/
std::unique_ptr<BaseRunner> HTTPRunner::Clone() const
{
    return std::make_unique<HTTPRunner>(
        GetFullURL(), mUserAgent, mBaseOptions, mHttpOptions);
}

/*****************************************************/
void HTTPRunner::InitializeClient(const std::string& protoHost)
{
    mHttpClient = std::make_unique<httplib::Client>(protoHost);

    if (mHttpOptions.followRedirects)
        mHttpClient->set_follow_location(true);

    mHttpClient->set_keep_alive(true);
    mHttpClient->set_read_timeout(mBaseOptions.timeout);
    mHttpClient->set_write_timeout(mBaseOptions.timeout);

    mHttpClient->enable_server_certificate_verification(mHttpOptions.tlsCertVerify);

    if (!mHttpOptions.username.empty())
    {
        mHttpClient->set_basic_auth(
            mHttpOptions.username, mHttpOptions.password);
    }

    if (!mHttpOptions.proxyHost.empty())
    {
        mHttpClient->set_proxy(
            mHttpOptions.proxyHost, mHttpOptions.proxyPort);
    }

    if (!mHttpOptions.proxyUsername.empty())
    {
        mHttpClient->set_proxy_basic_auth(
            mHttpOptions.proxyUsername, mHttpOptions.proxyPassword);
    }
}

/*****************************************************/
HTTPRunner::HostUrlPair HTTPRunner::ParseURL(const std::string& fullURL)
{
    const std::string fullerURL { 
        (fullURL.find("://") != std::string::npos)
            ? fullURL : ("http://"+fullURL) };

    StringUtil::StringPair pair { StringUtil::split(fullerURL, "/", 2) };
    if (!StringUtil::startsWith(pair.second,"/")) 
        pair.second.insert(0,"/");
    return pair;
}

/*****************************************************/
std::string HTTPRunner::GetHostname() const
{
    return StringUtil::split(mProtoHost, "://").second;
}

/*****************************************************/
std::string HTTPRunner::SetupRequest(const RunnerInput& input, httplib::Headers& headers, bool dataParams)
{
    headers.emplace("User-Agent", mUserAgent);

    // set up the URL parameters and query string
    httplib::Params urlParams {{"api",""},{"app",input.app},{"action",input.action}};

    // set up plainParams as URL variables (for server logging)
    for (const decltype(input.plainParams)::value_type& it : input.plainParams)
        urlParams.emplace(it); 

    // set up dataParams as base64-encoded X-Andromeda headers
    if (dataParams) for (const decltype(input.dataParams)::value_type& it : input.dataParams)
    {
        std::string key { it.first };
        std::replace(key.begin(), key.end(), '_', '-');
        headers.emplace("X-Andromeda-"+key, base64::encode(it.second));
    }

    return mBaseURL + (mBaseURL.find('?') != std::string::npos ? "&" : "?") + 
        httplib::detail::params_to_query_str(urlParams);
}

/*****************************************************/
void HTTPRunner::AddDataParams(const RunnerInput& input, httplib::MultipartFormDataItems& postParams)
{
    // set up dataParams as POST body inputs
    for (const decltype(input.dataParams)::value_type& it : input.dataParams)
        postParams.push_back({it.first, it.second, {}, {}});
}

/*****************************************************/
void HTTPRunner::AddFileParams(const RunnerInput_FilesIn& input, httplib::MultipartFormDataItems& postParams)
{
    for (const decltype(input.files)::value_type& it : input.files)
        postParams.push_back({it.first, it.second.data, it.second.name, {}});
}

// We RETRY if either httplib gives no response (can't connect, etc.) or if we
// get a response but it's a HTTP 503.  Other responses (404 etc.) don't get retried.

/*****************************************************/
void HTTPRunner::DoRequests(const std::function<httplib::Result()>& getResult, bool& canRetry, const bool& doRetry)
{
    // do the request some number of times until success
    for (decltype(mBaseOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        canRetry = (GetCanRetry() && attempt <= mBaseOptions.maxRetries);
        httplib::Result result { getResult() }; // sets doRetry
        if (result != nullptr && !doRetry) return; // break
        else HandleNonResponse(result, canRetry, attempt);
    }
}

/*****************************************************/
std::string HTTPRunner::DoRequestsFull(const std::function<httplib::Result()>& getResult, bool& isJson)
{
    // do the request some number of times until success
    for (decltype(mBaseOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        const bool canRetry = (GetCanRetry() && attempt <= mBaseOptions.maxRetries);
        httplib::Result result { getResult() };
        if (result != nullptr)
        {
            bool doRetry = false; std::string retval { 
                HandleResponse(*result, isJson, canRetry, doRetry) };
            if (!doRetry) return retval; // break
        }
        // if doRetry is set by HandleResponse, continue here
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
            else str << httplib::to_string(result.error());

            str << " error, attempt " << attempt+1 << " of " << mBaseOptions.maxRetries+1;
        });

        if (attempt != 0) // retry immediately after 1st failure
            std::this_thread::sleep_for(mBaseOptions.retryTime);
    }
    else if (result.error() == httplib::Error::Connection)
            throw ConnectionException();
    else throw LibraryException(result.error());
}

/*****************************************************/
std::string HTTPRunner::HandleResponse(const httplib::Response& response, bool& isJson, const bool& canRetry, bool& doRetry)
{
    MDBG_INFO("() HTTP:" << response.status);

    doRetry = (canRetry && (response.status == 500 || response.status == 503)); // TODO remove me (don't retry on 500)
    if (doRetry) return ""; // early return

    // if redirected, should remember it for next time
    if (mHttpOptions.followRedirects && !response.location.empty()) 
        RegisterRedirect(response.location);

    switch (response.status)
    {
        case 200: // OK
        {
            isJson = (response.has_header("Content-type") && 
                response.get_header_value("Content-type") == "application/json");
            return response.body; // copy from const
        }
        
        case 301: case 302: // HTTP redirect
            throw GetRedirectException(response); break;

        case 400: throw EndpointException("400 Bad Request");
        case 403: throw EndpointException("403 Access Denied");
        case 404: throw EndpointException("404 Not Found");
        case 413: throw InputSizeException(); // can be handled
        case 500: throw EndpointException("500 Server Error");
        case 503: throw EndpointException("503 Server Overloaded");
        default:  throw EndpointException(response.status);
    }
}

/*****************************************************/
std::string HTTPRunner::RunAction_Read(const RunnerInput& input, bool& isJson)
{
    MDBG_INFO("()");

    httplib::Headers headers; 
    std::string url(SetupRequest(input, headers));

    return DoRequestsFull([&](){ return mHttpClient->Get(url, headers); }, isJson);
}

/*****************************************************/
std::string HTTPRunner::RunAction_Write(const RunnerInput& input, bool& isJson)
{
    MDBG_INFO("()");

    httplib::Headers headers; 
    httplib::MultipartFormDataItems postParams;
    std::string url(SetupRequest(input, headers, false));

    AddDataParams(input, postParams);

    return DoRequestsFull([&](){ return mHttpClient->Post(url, headers, postParams); }, isJson);
}

/*****************************************************/
std::string HTTPRunner::RunAction_FilesIn(const RunnerInput_FilesIn& input, bool& isJson)
{
    MDBG_INFO("()");

    // set up the POST body as multipart files
    httplib::Headers headers;
    httplib::MultipartFormDataItems postParams;
    std::string url(SetupRequest(input, headers, false)); 

    AddDataParams(input, postParams);
    AddFileParams(input, postParams);

    return DoRequestsFull([&](){ return mHttpClient->Post(url, headers, postParams); }, isJson);
}

/*****************************************************/
std::string HTTPRunner::RunAction_StreamIn(const RunnerInput_StreamIn& input, bool& isJson)
{
    MDBG_INFO("()");

    httplib::Headers headers;
    httplib::MultipartFormDataItems postParams;
    std::string url(SetupRequest(input, headers, false)); 

    AddDataParams(input, postParams);
    AddFileParams(input, postParams);
    
    // set up the POST body as files being done via a chunked stream
    httplib::MultipartFormDataProviderItems streamParams;

    for (const decltype(input.fstreams)::value_type& it : input.fstreams)
    {
        // callback that reads from the provided file stream into our buffer then httplib's buffer
        const httplib::ContentProviderWithoutLength sfunc { [&](size_t offset, httplib::DataSink& sink)->bool
        {
            size_t read { 0 };
            const bool hasMore { it.second.streamer(offset, mStreamBuffer.data(), mStreamBuffer.size(), read) };
            sink.os.write(mStreamBuffer.data(), static_cast<std::streamsize>(read));
            if (!hasMore) sink.done(); 
            return true;
        }};

        streamParams.push_back({it.first, sfunc, it.second.name, {}});
    }
    
    return DoRequestsFull([&](){ return mHttpClient->Post(url, headers, postParams, streamParams); }, isJson);
}

/*****************************************************/
void HTTPRunner::RunAction_StreamOut(const RunnerInput_StreamOut& input, bool& isJson)
{
    MDBG_INFO("()");

    // httplib only supports ContentReceiver with Get() so use headers instead of MultiPart
    httplib::Headers headers; 
    std::string url(SetupRequest(input, headers));

    // these bools are set by the handlers but we'll give defaults anyway
    bool canRetry = true, doRetry = false;
    size_t offset = 0;

    // Separate ResponseHandler callback as we need to check the response before streaming
    httplib::ResponseHandler respFunc { [&](const httplib::Response& response)->bool {
        HandleResponse(response, isJson, canRetry, doRetry); 
        offset = 0; return true; // reset offset in case of retry
    }};

    httplib::ContentReceiver recvFunc { [&](const char* data, size_t length)->bool {
        input.streamer(offset, data, length); 
        offset += length; return true;
    }};

    // First we call DoRequests which will set canRetry (by ref), then runs the httplib Get(). 
    // httplib then calls our ResponseHandler, which checks the response and canRetry, then sets doRetry if needed.  
    // httplib then returns to DoRequests which checks the doRetry and starts over if set.

    DoRequests([&](){ return mHttpClient->Get(url, headers, respFunc, recvFunc); }, canRetry, doRetry);
}

/*****************************************************/
void HTTPRunner::RegisterRedirect(const std::string& location)
{
    MDBG_INFO("(location:" << location << ")");

    HostUrlPair newPair { ParseURL(location) };

    const size_t paramsPos { newPair.second.find("?") };
    if (paramsPos != std::string::npos) // remove URL params
        newPair.second.erase(paramsPos);
    
    if (newPair.first != mProtoHost)
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
        
        const size_t paramsPos { location.find('?') };
        if (paramsPos != std::string::npos) // remove URL params
            location.erase(paramsPos);

        return RedirectException(location);
    }
    else return RedirectException();
}

} // namespace Backend
} // namespace Andromeda
