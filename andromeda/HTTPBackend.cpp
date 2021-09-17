#include <sstream>
#include <string>
#include <utility>

#include "HTTPBackend.hpp"
#include "Utilities.hpp"

HTTPBackend::HTTPBackend(const std::string& hostname, const std::string& baseURL) : 
    debug("HTTPBackend",this), baseURL(baseURL), httpClient(hostname)
{
    debug << __func__ << "(hostname:" << hostname << " baseURL:" << baseURL << ")"; debug.Print();

    this->httpClient.set_keep_alive(true);
}

std::string HTTPBackend::RunAction(const std::string& app, const std::string& action, const Params& params)
{
    httplib::Params urlParams {{"app",app},{"action",action}};

    std::string url(this->baseURL + "?" + httplib::detail::params_to_query_str(urlParams));

    httplib::Params postParams;

    for (Params::const_iterator it = params.cbegin(); it != params.cend(); it++)
        postParams.insert(httplib::Params::value_type(it->first, it->second));  
    
    httplib::Result response = this->httpClient.Post(url.c_str(), postParams);

    if (!response) throw LibErrorException(response.error());

    // TODO look into more httplib functionality like reading into functions

    debug << __func__ << "(...) HTTP:" << std::to_string(response->status); debug.Print();

    switch (response->status)
    {
        case 200: return std::move(response->body); 
        case 404: throw NotFoundException(); break;
        default: throw Exception(response->status); break;
    }
}
