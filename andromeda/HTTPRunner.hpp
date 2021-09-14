#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include "Runner.hpp"

#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#define CPPHTTPLIB_ZLIB_SUPPORT 1
#include "httplib.h"

class LibErrorException : public RunnerException { 
    public: LibErrorException(httplib::Error error) : 
        RunnerException("httplib error: "+httplib::to_string(error)) {} };

class HTTPRunner : public Runner
{
public:
    HTTPRunner(const std::string& hostname, const std::string& baseURL);
    
    virtual std::string RunAction(const std::string& app, const std::string& action) override;

private:
    const std::string baseURL;
    httplib::Client httpClient;
};

#endif