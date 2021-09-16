#include <sstream>
#include <string>
#include <utility>

#include "HTTPBackend.hpp"
#include "Utilities.hpp"

HTTPBackend::HTTPBackend(const std::string& hostname, const std::string& baseURL) : 
    debug("HTTPBackend",this), baseURL(baseURL), httpClient(hostname)
{
    debug << __func__ << "(hostname:" << hostname << " baseURL:" << baseURL << ")"; debug.Print();

    this->httpClient.set_compress(true); // TODO is this enough or do we need the HTTP header?
    this->httpClient.set_keep_alive(true);
}

std::string HTTPBackend::RunAction(const std::string& app, const std::string& action)
{    
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Print();

    std::ostringstream url; url << "/" << this->baseURL << "?app=" << app << "&action=" << action;

    this->debug << __func__ << "(): fullURL:" << url.str(); this->debug.Print();

    httplib::Result response = this->httpClient.Get(url.str().c_str());

    if (!response) throw LibErrorException(response.error());

    // TODO look into more httplib functionality like reading into functions

    switch (response->status)
    {
        case 200: return std::move(response->body); 
        case 404: throw NotFoundException(); break;
        default: throw Exception(response->status); break;
    }
}
