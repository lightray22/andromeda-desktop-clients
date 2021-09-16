#ifndef LIBA2_OPTIONS_H_
#define LIBA2_OPTIONS_H_

#include <stdexcept>
#include <string>

#include "Utilities.hpp"

class Options
{
public:

    class Exception : public Utilities::Exception { 
        using Utilities::Exception::Exception; };

    class BadUsageException : public Exception { 
        public: BadUsageException() : 
            Exception("Invalid Usage") {} };

    class BadFlagException : public Exception { 
        public: BadFlagException(const std::string& flag) : 
            Exception("Unknown Flag: "+flag) {} };

    class BadOptionException : public Exception { 
        public: BadOptionException(const std::string& option) : 
            Exception("Unknown Option: "+option) {} };

    class BadValueException : public Exception {
        public: BadValueException(const std::string& option) :
            Exception("Bad Option Value: "+option) {} };

    class MissingOptionException : public Exception {
        public: MissingOptionException(const std::string& option) :
            Exception("Missing Option: "+option) {} };

    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    static std::string HelpText();

    void Parse(int argc, char** argv);

    Debug::Level GetDebugLevel() { return this->debug; }

    bool HasUsername() { return !this->username.empty(); }
    std::string GetUsername() { return this->username; }

    ApiType GetApiType() { return this->apiType; }    
    std::string GetApiPath() { return this->apiPath; }
    std::string GetApiHostname() { return this->apiHostname; }

    std::string GetMountPath() { return this->mountPath; }

private:
    
    Debug::Level debug = Debug::Level::DEBUG_NONE;

    std::string username;

    ApiType apiType;
    std::string apiPath;
    std::string apiHostname;

    std::string mountPath;
};

#endif