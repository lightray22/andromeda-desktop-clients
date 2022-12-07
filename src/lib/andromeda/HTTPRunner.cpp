#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "HTTPRunner.hpp"
#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& protoHost, const std::string& baseURL, const HTTPRunner::Options& options) : 
    mDebug("HTTPRunner",this), mOptions(options), mProtoHost(protoHost), mBaseURL(baseURL)
{
    if (mBaseURL.find("/") != 0) mBaseURL.insert(0, "/");

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

    if (pair.second.find("/") != 0) pair.second.insert(0, "/");

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
std::string HTTPRunner::RunAction(const Backend::Runner::Input& input)
{
    httplib::Params urlParams {{"api",""},{"app",input.app},{"action",input.action}};

    std::string sep(mBaseURL.find("?") != std::string::npos ? "&" : "?");

    std::string url(mBaseURL + sep + 
        httplib::detail::params_to_query_str(urlParams));

    httplib::MultipartFormDataItems postParams;

    for (const Params::value_type& it : input.params)
        postParams.push_back({it.first, it.second, {}, {}});

    for (const Files::value_type& it : input.files)
        postParams.push_back({it.first, it.second.data, it.second.name, {}});

    for (decltype(mOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        httplib::Result response(mHttpClient->Post(url.c_str(), postParams));

        if (!response || response->status == 503) // 503 is temporary 
        {
            if (mCanRetry && attempt <= mOptions.maxRetries)
            {
                mDebug << __func__ << "... ";
                
                if (response) mDebug << "HTTP " << response->status;
                else mDebug << httplib::to_string(response.error());

                mDebug << " error, attempt " << attempt+1 << " of " << mOptions.maxRetries+1; mDebug.Error();

                std::this_thread::sleep_for(mOptions.retryTime); continue;
            }
            else if (response) // got a response
                throw EndpointException(response->status);
            else if (response.error() == httplib::Error::Connection)
                 throw ConnectionException();
            else throw Exception(response.error());
        }

        mDebug << __func__ << "... HTTP:" << response->status; mDebug.Info();

        if (mOptions.followRedirects && !response->location.empty()) 
            HandleRedirect(response->location);

        switch (response->status)
        {
            case 200: return std::move(response->body);
            
            case 301: case 302: // HTTP redirect
            {
                std::string extext { "Redirected" };
                auto location { response->headers.find("Location") };
                if (location != response->headers.end())
                    extext += ": "+location->second;
                throw EndpointException(extext);
            }

            case 403: throw EndpointException("Access Denied");
            case 404: throw EndpointException("Not Found");
            default:  throw EndpointException(response->status);
        }
    }
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
        mDebug << __func__ << "... newProtoHost:" << newPair.first; mDebug.Info();
        mProtoHost = newPair.first;
        InitializeClient(mProtoHost);
    }

    if (newPair.second != mBaseURL)
    {
        mDebug << __func__ << "... newBaseURL:" << newPair.second; mDebug.Info();
        mBaseURL = newPair.second;
    }
}

} // namespace Andromeda
