
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "HTTPRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& protoHost, const std::string& baseURL, const HTTPOptions& options) : 
    mDebug("HTTPRunner",this), mOptions(options), mProtoHost(protoHost), mBaseURL(baseURL)
{
    if (!Utilities::startsWith(mBaseURL,"/")) mBaseURL.insert(0, "/");

    mDebug << __func__ << "(protoHost:" << mProtoHost << " baseURL:" << mBaseURL << ")"; mDebug.Info();

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
std::string HTTPRunner::RunAction(const RunnerInput& input, bool& isJson)
{
    // set up the URL parameters and query string
    httplib::Params urlParams {{"api",""},{"app",input.app},{"action",input.action}};

    std::string url(mBaseURL + (mBaseURL.find("?") != std::string::npos ? "&" : "?") + 
        httplib::detail::params_to_query_str(urlParams));

    // set up POST body parameters and files
    httplib::MultipartFormDataItems postParams;

    for (const decltype(input.params)::value_type& it : input.params)
        postParams.push_back({it.first, it.second, {}, {}});

    for (const decltype(input.files)::value_type& it : input.files)
        postParams.push_back({it.first, it.second.data, it.second.name, {}});

    // set up POST body files being done via a chunked stream
    httplib::MultipartFormDataProviderItems streamParams;

    std::unique_ptr<char[]> streamBuffer { !input.sfiles.empty()
        ? std::make_unique<char[]>(mOptions.streamBufferSize) : nullptr };

    for (const decltype(input.sfiles)::value_type& it : input.sfiles)
    {
        httplib::ContentProviderWithoutLength sfunc { GetStreamFunc(
            streamBuffer.get(), mOptions.streamBufferSize, it.second.data) };
        streamParams.push_back({it.first, sfunc, it.second.name, {}});
    }

    // do the request some number of times
    for (decltype(mOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        httplib::Result response { streamParams.empty() 
            ? mHttpClient->Post(url.c_str(), httplib::Headers(), postParams)
            : mHttpClient->Post(url.c_str(), httplib::Headers(), postParams, streamParams) };

        // if no response or got 503, potentially retry
        if (!response || response->status == 503)
        {
            if (mCanRetry && attempt <= mOptions.maxRetries)
            {
                mDebug << __func__ << "... ";
                
                if (response) mDebug << "HTTP " << response->status;
                else mDebug << httplib::to_string(response.error());

                mDebug << " error, attempt " << attempt+1 << " of " << mOptions.maxRetries+1; mDebug.Error();

                std::this_thread::sleep_for(mOptions.retryTime); continue;
            }
            // got a response, treat like any other error below
            else if (response) { }
            // otherwise throw an exception based on the error
            else if (response.error() == httplib::Error::Connection)
                 throw ConnectionException();
            else throw LibraryException(response.error());
        }

        mDebug << __func__ << "... HTTP:" << response->status; mDebug.Info();

        // if redirected, should remember it for next time
        if (mOptions.followRedirects && !response->location.empty()) 
            HandleRedirect(response->location);

        switch (response->status)
        {
            case 200: // OK
            {
                auto contentIt { response->headers.find("Content-type") };
                isJson = (contentIt != response->headers.end() 
                    && contentIt->second == "application/json");
                return std::move(response->body);
            }
            
            case 301: case 302: // HTTP redirect
                RedirectException(response->headers); break;
            
            case 403: throw EndpointException("Access Denied");
            case 404: throw EndpointException("Not Found");
            default:  throw EndpointException(response->status);
        }
    }
}

/*****************************************************/
httplib::ContentProviderWithoutLength HTTPRunner::GetStreamFunc(
    char* const& buf, const size_t& bufSize, std::istream& in)
{
    return [&](size_t offset, httplib::DataSink& sink)->bool
    {
        in.read(buf, static_cast<std::streamsize>(bufSize));
        sink.os.write(buf, in.gcount());

        mDebug << __func__ << "... read " << in.gcount() << " bytes into " << bufSize << " buffer"; mDebug.Info();
        mDebug << __func__ << "... eof=" << in.eof() << " fail=" << in.fail() << " bad=" << in.bad(); mDebug.Info();

        if (in.eof()) sink.done();

        return !in.bad() && (!in.fail() || in.eof());
    };
}

/*****************************************************/
void HTTPRunner::HandleRedirect(const std::string& location)
{
    mDebug << __func__ << "(location:" << location << ")"; mDebug.Info();

    HTTPRunner::HostUrlPair newPair { HTTPRunner::ParseURL(location) };

    const size_t paramsPos { newPair.second.find("?") };
    if (paramsPos != std::string::npos) // remove URL params
        newPair.second.erase(paramsPos);
    
    if (newPair.first != GetProtoHost())
    {
        mDebug << __func__ << "... new protoHost:" << newPair.first; mDebug.Info();
        mProtoHost = newPair.first;
        InitializeClient(mProtoHost);
    }

    if (newPair.second != mBaseURL)
    {
        mDebug << __func__ << "... new baseURL:" << newPair.second; mDebug.Info();
        mBaseURL = newPair.second;
    }
}

/*****************************************************/
void HTTPRunner::RedirectException(const httplib::Headers& headers)
{
    std::string extext { "Redirected" };

    auto locationIt { headers.find("Location") };
    if (locationIt != headers.end())
    {
        std::string location { locationIt->second };
        
        const size_t paramsPos { location.find("?") };
        if (paramsPos != std::string::npos) // remove URL params
            location.erase(paramsPos);

        extext += ": "+location;
    }

    throw EndpointException(extext);
}

} // namespace Backend
} // namespace Andromeda
