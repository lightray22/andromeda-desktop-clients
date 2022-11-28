#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "HTTPRunner.hpp"
#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& protoHost, const std::string& baseURL, const HTTPRunner::Options& options) : 
    mDebug("HTTPRunner",this), mOptions(options), mProtoHost(protoHost), mBaseURL(baseURL), mHttpClient(protoHost)
{
    mDebug << __func__ << "(protoHost:" << protoHost << " baseURL:" << baseURL << ")"; mDebug.Info();

    mHttpClient.set_keep_alive(true);

    mHttpClient.set_read_timeout(mOptions.timeout);
    mHttpClient.set_write_timeout(mOptions.timeout);

    if (!mOptions.username.empty())
    {
        mHttpClient.set_basic_auth(
            mOptions.username.c_str(), mOptions.password.c_str());
    }

    if (!mOptions.proxyHost.empty())
    {
        mHttpClient.set_proxy(
            mOptions.proxyHost.c_str(), mOptions.proxyPort);
    }

    if (!mOptions.proxyUsername.empty())
    {
        mHttpClient.set_proxy_basic_auth(
            mOptions.proxyUsername.c_str(), mOptions.proxyPassword.c_str());
    }
}

/*****************************************************/
HTTPRunner::HostUrlPair HTTPRunner::ParseURL(std::string fullURL)
{
    if (fullURL.find("://") == std::string::npos)
        fullURL.insert(0, "http://");

    HTTPRunner::HostUrlPair retval { 
        Utilities::split(fullURL, "/", 2) };

    if (retval.second.empty() || retval.second[0] != '/')
        retval.second.insert(0,"/");

    if (Utilities::endsWith(retval.second, "/"))
        retval.second += "index.php";
    
    if (!Utilities::endsWith(retval.second, "/index.php"))
        retval.second += "/index.php";
    
    return retval;
}

/*****************************************************/
std::string HTTPRunner::GetHostname() const
{
    return Utilities::split(mProtoHost, "://").second;
}

/*****************************************************/
std::string HTTPRunner::RunAction(const Backend::Runner::Input& input)
{
    httplib::Params urlParams {{"app",input.app},{"action",input.action}};

    std::string sep(mBaseURL.find("?") != std::string::npos ? "&" : "?");

    std::string url(mBaseURL + sep + 
        httplib::detail::params_to_query_str(urlParams));

    httplib::MultipartFormDataItems postParams;

    for (const Params::value_type& it : input.params)
    {
        postParams.push_back({it.first, it.second, {}, {}});
    }

    for (const Files::value_type& it : input.files)
    {
        postParams.push_back({it.first, it.second.data, it.second.name, {}});
    }

    for (decltype(mOptions.maxRetries) attempt { 0 }; ; attempt++)
    {
        httplib::Result response(mHttpClient.Post(url.c_str(), postParams));

        if (!response || response->status >= 500) 
        {
            if (attempt <= mOptions.maxRetries && mCanRetry)
            {
                mDebug << __func__ << "... ";
                
                if (response) mDebug << "HTTP " << response->status;
                else mDebug << httplib::to_string(response.error());

                mDebug << " error, attempt " << attempt+1 << " of " << mOptions.maxRetries+1; mDebug.Error();

                std::this_thread::sleep_for(mOptions.retryTime); continue;
            }
            else if (response.error() == httplib::Error::Connection)
                 throw ConnectionException();
            else if (response) throw EndpointException(response->status);
            else throw Exception(response.error());
        }

        mDebug << __func__ << "... HTTP:" << response->status; mDebug.Info();

        switch (response->status)
        {
            case 200: return std::move(response->body);
            case 403: throw EndpointException("Access Denied");
            case 404: throw EndpointException("Not Found");
            default:  throw EndpointException(response->status);
        }
    }
}

} // namespace Andromeda
