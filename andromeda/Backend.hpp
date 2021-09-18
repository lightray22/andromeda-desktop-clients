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

    typedef std::map<std::string, std::string> Params;

    class Runner
    {
    public:
        virtual std::string RunAction(
            const std::string& app, const std::string& action, 
            const Backend::Params& params = Backend::Params()) = 0;
    };

    class Exception : public Utilities::Exception { public:
        Exception(int code) : 
            Utilities::Exception("Backend Error: HTTP "+std::to_string(code)){};
        Exception(const std::string& message) :
            Utilities::Exception("Backend Error: "+message) {}; };

    class JSONErrorException : public Exception { public:
        JSONErrorException(const std::string& error) : 
            Exception("JSON Error: "+error) {}; };
    
    class APIException : public Exception { public:
        APIException(int code, const std::string& message) : 
            Exception("API code:"+std::to_string(code)+" message:"+message) {}; };

    class APIDeniedException : public Exception { public:
        APIDeniedException() : Exception("Access Denied") {};
        APIDeniedException(const std::string& message) : Exception(message) {}; };

    class APINotFoundException : public Exception { public:
        APINotFoundException() : Exception("Not Found") {};
        APINotFoundException(const std::string& message) : Exception(message) {}; };

    class AuthenticationFailedException : public APIDeniedException { public:
        AuthenticationFailedException() : APIDeniedException("Authentication Failed") {}; };

    class TwoFactorRequiredException : public APIDeniedException { public:
        TwoFactorRequiredException() : APIDeniedException("Two Factor Required") {}; };

    Backend(Runner& runner);

    virtual ~Backend();

    void Initialize();

    void Authenticate(const std::string& username, const std::string& password, const std::string& twofactor = "");

    void AuthInteractive(const std::string& username, std::string password = "", std::string twofactor = "");

    void PreAuthenticate(const std::string& sessionID, const std::string& sessionKey);

    nlohmann::json GetConfig();

private:

    std::string RunAction(const std::string& app, const std::string& action);
    std::string RunAction(const std::string& app, const std::string& action, Params& params);

    nlohmann::json GetJSON(const std::string& resp);

    bool createdSession = false;

    std::string sessionID;
    std::string sessionKey;

    Debug debug;
    Runner& runner;
    Config config;
};

#endif