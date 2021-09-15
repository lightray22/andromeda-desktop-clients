#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include "Runner.hpp"
#include "Utilities.hpp"

#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#define CPPHTTPLIB_ZLIB_SUPPORT 1
#include "httplib.h"

class HTTPRunner : public Runner
{
public:
    class LibErrorException : public Exception { 
        public: LibErrorException(httplib::Error error) : 
            Exception(httplib::to_string(error)) {} };

    HTTPRunner(const std::string& hostname, const std::string& baseURL);
    
    virtual std::string RunAction(const std::string& app, const std::string& action) override;

private:
    Debug debug;
    const std::string baseURL;
    httplib::Client httpClient;
};

#endif