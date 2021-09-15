
#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include "Runner.hpp"
#include "Config.hpp"
#include "Utilities.hpp"

#include <string>
#include <nlohmann/json_fwd.hpp>

class Backend
{
public:

    class Exception : public AndromedaException { public: 
        Exception() : AndromedaException("backend error") {};
        Exception(const std::string& message) : 
            AndromedaException("backend error: "+message) {}; };

    class JSONErrorException : public Exception { public:
        JSONErrorException(const std::string& error) : 
            Exception("json error: "+error) {}; };

    class NotFoundException : public Exception { public:
        NotFoundException() : Exception("not found") {}; };

    Backend(Runner& runner);

    void Authenticate(const std::string& username);

private:

    nlohmann::json RunAction(const std::string& app, const std::string& action);

    Debug debug;
    Runner& runner;
    Config config;
};

#endif