#include <sstream>
#include <string>
#include <utility>

#include "HTTPRunner.hpp"
#include "Utilities.hpp"

/*****************************************************/
HTTPRunner::HTTPRunner(const std::string& hostname, const std::string& baseURL) : 
    debug("HTTPRunner",this), baseURL(baseURL), httpClient(hostname)
{
    debug << __func__ << "(hostname:" << hostname << " baseURL:" << baseURL << ")"; debug.Info();

    this->httpClient.set_keep_alive(true);
}

/*****************************************************/
std::string HTTPRunner::RunAction(
    const std::string& app, const std::string& action, 
    const Backend::Params& params)
{
    httplib::Params urlParams {{"app",app},{"action",action}};

    std::string url(this->baseURL + "?" + 
        httplib::detail::params_to_query_str(urlParams));

    httplib::Params postParams;

    for (Backend::Params::const_iterator it = params.cbegin(); it != params.cend(); it++)
        postParams.insert(httplib::Params::value_type(it->first, it->second));  
    
    httplib::Result response = this->httpClient.Post(url.c_str(), postParams);

    if (!response) throw LibErrorException(response.error());

    debug << __func__ << "... HTTP:" << std::to_string(response->status); debug.Details();

    switch (response->status)
    {
        case 200: return std::move(response->body); 
        case 403: throw Backend::DeniedException(); break;
        case 404: throw Backend::NotFoundException(); break;
        default: throw Backend::Exception(response->status); break;
    }
}
