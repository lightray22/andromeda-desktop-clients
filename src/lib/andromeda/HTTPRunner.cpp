#include <sstream>
#include <string>
#include <utility>

#include "HTTPRunner.hpp"
#include "Utilities.hpp"

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& hostname, const std::string& baseURL, const HTTPRunner::Options& options) : 
    debug("HTTPRunner",this), baseURL(baseURL), httpClient(hostname)
{
    debug << __func__ << "(hostname:" << hostname << " baseURL:" << baseURL << ")"; debug.Info();

    //this->httpClient.set_keep_alive(true); - BUGGED
    //https://github.com/yhirose/cpp-httplib/issues/1041

    this->httpClient.set_read_timeout(60);
    this->httpClient.set_write_timeout(60);

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
    
    httplib::Result response(this->httpClient.Post(url.c_str(), postParams));

    if (!response) throw LibErrorException(response.error());

    debug << __func__ << "... HTTP:" << response->status; debug.Info();

    switch (response->status)
    {
        case 200: return std::move(response->body);
        case 403: throw EndpointException("Access Denied");
        case 404: throw EndpointException("Not Found");
        default:  throw EndpointException(response->status);
    }
}
