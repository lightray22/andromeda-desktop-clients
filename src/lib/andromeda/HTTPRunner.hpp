#ifndef LIBA2_HTTPRUNNER_H_
#define LIBA2_HTTPRUNNER_H_

#include <string>

#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "httplib.h"

#include "Backend.hpp"
#include "Utilities.hpp"

/** Runs the API remotely over HTTP */
class HTTPRunner : public Backend::Runner
{
public:

    /** Exception indicating the HTTP library had an error */
    class LibErrorException : public Backend::Exception { 
        public: LibErrorException(httplib::Error error) : 
            Exception(httplib::to_string(error)) {} };

    /**
     * @param hostname to use with HTTP
     * @param baseURL URL of the endpoint
     */
    HTTPRunner(const std::string& hostname, const std::string& baseURL);
    
    virtual std::string RunAction(const Input& input) override;

private:
    Debug debug;
    const std::string baseURL;
    httplib::Client httpClient;
};

#endif