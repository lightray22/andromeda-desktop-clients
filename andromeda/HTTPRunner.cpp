#include "HTTPRunner.hpp"

#include <iostream>
#include <sstream>

#include <list>
#include <string>
#include <utility>

typedef enum
{
    SUCCESS = 200,
    NOT_FOUND = 404
} HTTPCode;

HTTPRunner::HTTPRunner(const std::string& hostname, const std::string& baseURL) : 
    httpClient(hostname), baseURL(baseURL)
{
    std::cout << "RUNNER: hostname " << hostname << " baseURL " << baseURL << std::endl; // TODO global debug

    httpClient.set_compress(true); // TODO is this enough or do we need the header?
    httpClient.set_keep_alive(true);
}

std::string HTTPRunner::RunAction(const std::string& app, const std::string& action)
{
    std::ostringstream url; url << "/" << this->baseURL << "?app=" << app << "&action=" << action;

    std::cout << "\tHTTP RUN URL is " << url.str() << std::endl; // TODO global debug

    httplib::Result response = httpClient.Get(url.str().c_str());

    if (!response) throw LibErrorException(response.error());

    // TODO look into more httplib functionality like reading into functions

    switch (response->status)
    {
        case SUCCESS: return std::move(response->body);
        case NOT_FOUND: throw APINotFoundException("not found");
        default: throw APIErrorException("here's a message");
    }
}
