
#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include <map>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Config.hpp"
#include "Utilities.hpp"

class Backend
{
public:

    class Exception : public Utilities::Exception { public:
        Exception(int code) : 
            Utilities::Exception("Backend Error: HTTP "+std::to_string(code)){};
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

    nlohmann::json GetConfig();

    typedef std::map<std::string, std::string> Params;

protected:

    virtual std::string RunAction(const std::string& app, const std::string& action, const Params& params = Params()) = 0;

private:
    std::string RunBinaryAction(const std::string& app, const std::string& action, const Params& params = Params());
    nlohmann::json RunJsonAction(const std::string& app, const std::string& action, const Params& params = Params());

    Debug debug;
    Config config;
};

#endif