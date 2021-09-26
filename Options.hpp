#ifndef LIBA2_OPTIONS_H_
#define LIBA2_OPTIONS_H_

#include <utility>
#include <stdexcept>
#include <string>
#include <list>

#include "Utilities.hpp"

/** Manages command line options and config */
class Options
{
public:

    /** Base class for all Options errors */
    class Exception : public Utilities::Exception { 
        using Utilities::Exception::Exception; };

    /** Exception indicating help text should be shown */
    class ShowHelpException : public Exception {
        public: ShowHelpException() : Exception("") {} };

    /** Exception indicating the version should be shown */
    class ShowVersionException : public Exception {
        public: ShowVersionException() : Exception("") {} };

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

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Parses command line arguments from main */
    void Parse(int argc, char** argv);

    /** Returns the desired debug level */
    Debug::Level GetDebugLevel() const { return this->debugLevel; }

    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    /** Returns the specified API type */
    ApiType GetApiType() const { return this->apiType; }    

    /** Returns the path to the API endpoint */
    std::string GetApiPath() const { return this->apiPath; }

    /** Returns the API hostname if using API_URL */
    std::string GetApiHostname() const { return this->apiHostname; }

    /** True if a username was specified */
    bool HasUsername() const { return !this->username.empty(); }

    /** Returns the specified username */
    std::string GetUsername() const { return this->username; }

    enum class ItemType
    {
        SUPERROOT,
        FILESYSTEM,
        FOLDER
    };

    /** Returns the specified mount item type */
    ItemType GetMountItemType() const { return this->mountItemType; }

    /** Returns the specified mount item ID */
    std::string GetMountItemID() const { return this->mountItemID; }

    /** Returns the FUSE directory to mount */
    std::string GetMountPath() const { return this->mountPath; }

    const std::list<std::string>& GetFuseOptions() const { return this->fuseOptions; }

private:

    Debug::Level debugLevel = Debug::Level::NONE;

    ApiType apiType;
    std::string apiPath;
    std::string apiHostname;

    std::string username;
    
    ItemType mountItemType = ItemType::SUPERROOT;
    std::string mountItemID;

    std::string mountPath;

    std::list<std::string> fuseOptions;
};

#endif