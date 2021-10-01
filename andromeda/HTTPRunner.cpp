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

    this->httpClient.set_read_timeout(60, 0);
    this->httpClient.set_write_timeout(60, 0);
}

/*****************************************************/
std::string HTTPRunner::RunAction(const Backend::Runner::Input& input)
{
    httplib::Params urlParams {{"app",input.app},{"action",input.action}};

    std::string url(this->baseURL + "?" + 
        httplib::detail::params_to_query_str(urlParams));

    httplib::MultipartFormDataItems postParams;

    for (Params::const_iterator it = input.params.cbegin(); it != input.params.cend(); it++)
    {
        postParams.push_back({it->first, it->second});
    }

    for (Files::const_iterator it = input.files.cbegin(); it != input.files.cend(); it++)
    {
        postParams.push_back({it->first, it->second.data, it->second.name});
    }
    
    httplib::Result response = this->httpClient.Post(url.c_str(), postParams);

    if (!response) throw LibErrorException(response.error());

    debug << __func__ << "... HTTP:" << response->status; debug.Details();

    switch (response->status)
    {
        case 200: return std::move(response->body); 
        case 403: throw Backend::DeniedException(); break;
        case 404: throw Backend::NotFoundException(); break;
        default: throw Backend::Exception(response->status); break;
    }
}
