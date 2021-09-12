
#ifndef LIBA2_OPTIONS_H_
#define LIBA2_OPTIONS_H_

#include <exception>
#include <sstream>
#include <string>

class BadUsageException : public std::runtime_error { 
    public: BadUsageException() : runtime_error("Invalid Usage") {} };

class BadFlagException : public std::runtime_error { 
    public: BadFlagException(const std::string& flag) : 
        runtime_error("Unknown Flag: "+flag) {} };

class BadOptionException : public std::runtime_error { 
    public: BadOptionException(const std::string& option) : 
        runtime_error("Unknown Option: "+option) {} };

class BadValueException : public std::runtime_error {
    public: BadValueException(const std::string& option) :
        runtime_error("Bad Option Value: "+option) {} };

class MissingOptionException : public std::runtime_error {
    public: MissingOptionException(const std::string& option) :
        runtime_error("Missing Option: "+option) {} };

class Options
{
public:

    enum class DebugLevel
    {
        DEBUG_NONE,
        DEBUG_BASIC,
        DEBUG_DETAILS
    };

    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    static std::string HelpText();

    void Initialize(int argc, char** argv);

    DebugLevel GetDebugLevel() { return this->debug; }

    bool HasUsername() { return !this->username.empty(); }
    std::string GetUsername() { return this->username; }

    ApiType GetApiType() { return this->apiType; }
    std::string GetApiLocation() { return this->apiLoc; }

private:
    
    DebugLevel debug = DebugLevel::DEBUG_NONE;

    std::string username;

    ApiType apiType;
    std::string apiLoc;
};

#endif