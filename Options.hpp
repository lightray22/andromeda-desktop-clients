#ifndef LIBA2_OPTIONS_H_
#define LIBA2_OPTIONS_H_

#include <stdexcept>
#include <string>

#include "Utilities.hpp"

/** Manages command line options and config */
class Options
{
public:

    /** Base class for all Options errors */
    class Exception : public Utilities::Exception { 
        using Utilities::Exception::Exception; };

    /** Exception indicating the command usage was bad */
    class BadUsageException : public Exception { 
        public: BadUsageException() : 
            Exception("Invalid Usage") {} };

    /** Exception indicating the given flag is unknown */
    class BadFlagException : public Exception { 
        public: BadFlagException(const std::string& flag) : 
            Exception("Unknown Flag: "+flag) {} };

    /** Exception indicating the given option is unknown */
    class BadOptionException : public Exception { 
        public: BadOptionException(const std::string& option) : 
            Exception("Unknown Option: "+option) {} };

    /** Exception indicating the given option has a bad value */
    class BadValueException : public Exception {
        public: BadValueException(const std::string& option) :
            Exception("Bad Option Value: "+option) {} };

    /** Exception indicating the given option is required but not present */
    class MissingOptionException : public Exception {
        public: MissingOptionException(const std::string& option) :
            Exception("Missing Option: "+option) {} };

    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Parses command line arguments from main */
    void Parse(int argc, char** argv);

    /** Returns the desired debug level */
    Debug::Level GetDebugLevel() { return this->debugLevel; }

    /** Returns the specified API type */
    ApiType GetApiType() { return this->apiType; }    

    /** Returns the path to the API endpoint */
    std::string GetApiPath() { return this->apiPath; }

    /** Returns the API hostname if using API_URL */
    std::string GetApiHostname() { return this->apiHostname; }

    /** True if a username was specified */
    bool HasUsername() { return !this->username.empty(); }

    /** Returns the specified username */
    std::string GetUsername() { return this->username; }

    /** Returns the FUSE directory to mount */
    std::string GetMountPath() { return this->mountPath; }

private:

    Debug::Level debugLevel = Debug::Level::NONE;

    ApiType apiType;
    std::string apiPath;
    std::string apiHostname;

    std::string username;
    std::string mountPath;
};

#endif