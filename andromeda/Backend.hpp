
#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Config.hpp"
#include "Utilities.hpp"

class Backend
{
public:

    class Exception : public Utilities::Exception { public:
        Exception(int code) : 
            Utilities::Exception("Backend Error: HTTP "+code){};
        Exception(const std::string& message) :
            Utilities::Exception("Backend Error: "+message){}; };

    class NotFoundException : public Exception { public:
        NotFoundException() : 
            Exception("Not Found"){};
        NotFoundException(const std::string& message) : 
            Exception(message){}; };

    class JSONErrorException : public Exception { public:
        JSONErrorException(const std::string& error) : 
            Exception("JSON Error: "+error) {}; };

    class APIException : public Exception { public:
        APIException(int code, const std::string& message) : 
            Exception("API code:"+std::to_string(code)+" message:"+message) {}; };

    Backend();

    virtual ~Backend(){ };

    void Initialize();

    void Authenticate(const std::string& username);

    nlohmann::json GetServerConfig();

private:

    virtual std::string RunAction(const std::string& app, const std::string& action) = 0;

    nlohmann::json RunJsonAction(const std::string& app, const std::string& action);

    Debug debug;
    Config config;
};

#endif