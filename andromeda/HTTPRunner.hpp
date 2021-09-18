#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "Backend.hpp"
#include "Utilities.hpp"

class HTTPRunner : public Backend::Runner
{
public:
    class LibErrorException : public Backend::Exception { 
        public: LibErrorException(httplib::Error error) : 
            Exception(httplib::to_string(error)) {} };

    HTTPRunner(const std::string& hostname, const std::string& baseURL);
    
    virtual std::string RunAction(
        const std::string& app, const std::string& action, 
        const Backend::Params& params = Backend::Params()) override;

private:
    Debug debug;
    const std::string baseURL;
    httplib::Client httpClient;
};

#endif