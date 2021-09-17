#ifndef LIBA2_HTTPBACKEND_H_
#define LIBA2_HTTPBACKEND_H_

#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "Backend.hpp"
#include "Utilities.hpp"

class HTTPBackend : public Backend
{
public:
    class LibErrorException : public Exception { 
        public: LibErrorException(httplib::Error error) : 
            Exception(httplib::to_string(error)) {} };

    HTTPBackend(const std::string& hostname, const std::string& baseURL);
    
    virtual std::string RunAction(const std::string& app, const std::string& action, const Params& params = Params()) override;

private:
    Debug debug;
    const std::string baseURL;
    httplib::Client httpClient;
};

#endif