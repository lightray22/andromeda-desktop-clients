#ifndef LIBA2_OPTIONS_H_
#define LIBA2_OPTIONS_H_

#include <stdexcept>
#include <string>

class OptionsException : public std::runtime_error { 
    using std::runtime_error::runtime_error; };

class BadUsageException : public OptionsException { 
    public: BadUsageException() : 
        OptionsException("Invalid Usage") {} };

class BadFlagException : public OptionsException { 
    public: BadFlagException(const std::string& flag) : 
        OptionsException("Unknown Flag: "+flag) {} };

class BadOptionException : public OptionsException { 
    public: BadOptionException(const std::string& option) : 
        OptionsException("Unknown Option: "+option) {} };

class BadValueException : public OptionsException {
    public: BadValueException(const std::string& option) :
        OptionsException("Bad Option Value: "+option) {} };

class MissingOptionException : public OptionsException {
    public: MissingOptionException(const std::string& option) :
        OptionsException("Missing Option: "+option) {} };

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

    void Parse(int argc, char** argv);

    DebugLevel GetDebugLevel() { return this->debug; }

    bool HasUsername() { return !this->username.empty(); }
    std::string GetUsername() { return this->username; }

    ApiType GetApiType() { return this->apiType; }    
    std::string GetApiPath() { return this->apiPath; }
    std::string GetApiHostname() { return this->apiHostname; }

    std::string GetMountPath() { return this->mountPath; }

private:
    
    DebugLevel debug = DebugLevel::DEBUG_NONE;

    std::string username;

    ApiType apiType;
    std::string apiPath;
    std::string apiHostname;

    std::string mountPath;
};

#endif