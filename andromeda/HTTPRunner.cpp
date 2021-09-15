#include "HTTPRunner.hpp"
#include "Utilities.hpp"

#include <sstream>
#include <list>
#include <string>
#include <utility>

HTTPRunner::HTTPRunner(const std::string& hostname, const std::string& baseURL) : 
    debug("HTTPRunner",this), baseURL(baseURL), httpClient(hostname)
{
    debug << __func__ << ": hostname:" << hostname << " baseURL:" << baseURL; debug.Print();

    this->httpClient.set_compress(true); // TODO is this enough or do we need the HTTP header?
    this->httpClient.set_keep_alive(true);
}

std::string HTTPRunner::RunAction(const std::string& app, const std::string& action)
{
    std::ostringstream url; url << "/" << this->baseURL << "?app=" << app << "&action=" << action;

    this->debug << __func__ << ": fullURL:" << url.str(); this->debug.Print();

    httplib::Result response = this->httpClient.Get(url.str().c_str());

    if (!response) throw LibErrorException(response.error());

    // TODO look into more httplib functionality like reading into functions

    return std::move(response->body);
}
