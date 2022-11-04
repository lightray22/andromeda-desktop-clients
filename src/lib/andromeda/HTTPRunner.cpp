#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "HTTPRunner.hpp"
#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& hostname, const std::string& baseURL, const HTTPRunner::Options& opts) : 
    debug("HTTPRunner",this), options(opts), baseURL(baseURL), httpClient(hostname)
{
    debug << __func__ << "(hostname:" << hostname << " baseURL:" << baseURL << ")"; debug.Info();

    this->httpClient.set_keep_alive(true);

    this->httpClient.set_read_timeout(options.timeout);
    this->httpClient.set_write_timeout(options.timeout);

    if (!Utilities::endsWith(this->baseURL, "/index.php"))
        this->baseURL += "/index.php";

    if (!options.username.empty())
    {
        httpClient.set_basic_auth(
            options.username.c_str(), options.password.c_str());
    }

    if (!options.proxyHost.empty())
    {
        httpClient.set_proxy(
            options.proxyHost.c_str(), options.proxyPort);
    }

    if (!options.proxyUsername.empty())
    {
        httpClient.set_proxy_basic_auth(
            options.proxyUsername.c_str(), options.proxyPassword.c_str());
    }
}

/*****************************************************/
std::string HTTPRunner::RunAction(const Backend::Runner::Input& input)
{
    httplib::Params urlParams {{"app",input.app},{"action",input.action}};

    std::string sep(this->baseURL.find("?") != std::string::npos ? "&" : "?");

    std::string url(this->baseURL + sep + 
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

    for (size_t attempt = 0;; attempt++)
    {
        httplib::Result response(this->httpClient.Post(url.c_str(), postParams));

        if (!response || response->status >= 500) 
        {
            if (attempt <= options.maxRetries && this->canRetry)
            {
                debug << __func__ << "()... ";
                
                if (response) debug << "HTTP " << response->status;
                else debug << httplib::to_string(response.error());

                debug << " error, attempt " << attempt+1 << " of " << options.maxRetries+1; debug.Error();

                std::this_thread::sleep_for(options.retryTime); continue;
            }
            else if (response.error() == httplib::Error::Connection)
                 throw ConnectionException();
            else if (response) throw EndpointException(response->status);
            else throw Exception(response.error());
        }

        debug << __func__ << "()... HTTP:" << response->status; debug.Info();

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
