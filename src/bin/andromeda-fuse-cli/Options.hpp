#ifndef LIBA2_OPTIONS_H_
#define LIBA2_OPTIONS_H_

#include <filesystem>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>

#include "Config.hpp"
#include "FuseAdapter.hpp"
#include "HTTPRunner.hpp"
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
        /** @param flag the unknown flag */
        public: BadFlagException(const std::string& flag) : 
            Exception("Unknown Flag: "+flag) {} };

    /** Exception indicating the given option is unknown */
    class BadOptionException : public Exception { 
        /** @param option the unknown option */
        public: BadOptionException(const std::string& option) : 
            Exception("Unknown Option: "+option) {} };

    /** Exception indicating the given option has a bad value */
    class BadValueException : public Exception {
        /** @param option the option name */
        public: BadValueException(const std::string& option) :
            Exception("Bad Option Value: "+option) {} };

    /** Exception indicating the given option is required but not present */
    class MissingOptionException : public Exception {
        /** @param option the option name */
        public: MissingOptionException(const std::string& option) :
            Exception("Missing Option: "+option) {} };

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param cOpts Config options ref to fill
     * @param fOpts FUSE options ref to fill
     * @param hOpts HTTPRunner options ref to fill (if applicable)
     */
    Options(Config::Options& cOpts, FuseAdapter::Options& fOpts, HTTPRunner::Options& hOpts);

    /** Parses command line arguments from main */
    void ParseArgs(int argc, char** argv);

    /** Parses arguments from a config file */
    void ParseFile(const std::filesystem::path& path);

    /** Makes sure all required args were provided */
    void CheckMissing();

    /** Returns the desired debug level */
    Debug::Level GetDebugLevel() const { return this->debugLevel; }

    /** Backend connection type */
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

    /** Returns true if a username is specified */
    bool HasUsername() const { return !this->username.empty(); }

    /** Returns the specified username */
    std::string GetUsername() const { return this->username; }

    /** Returns true if a password is specified */
    bool HasPassword() const { return !this->password.empty(); }

    /** Returns the specified password */
    std::string GetPassword() const { return this->password; }

    /** Returns true if a session ID was provided */
    bool HasSession() const { return !this->sessionid.empty(); }

    /** Returns the specified session ID */
    std::string GetSessionID() const { return this->sessionid; }

    /** Returns the specified session key */
    std::string GetSessionKey() const { return this->sessionkey; }

    /** Returns true if using a session is forced */
    bool GetForceSession() const { return this->forceSession; }

    /** Folder types that can be mounted as root */
    enum class RootType
    {
        SUPERROOT,
        FILESYSTEM,
        FOLDER
    };

    /** Returns the specified mount item type */
    RootType GetMountRootType() const { return this->mountRootType; }

    /** Returns the specified mount item ID */
    std::string GetMountItemID() const { return this->mountItemID; }

private:

    /** Load config from the given flags and options */
    void LoadFrom(const Utilities::Flags& flags, const Utilities::Options options);

    Config::Options& cOptions;
    FuseAdapter::Options& fOptions;
    HTTPRunner::Options& hOptions;

    Debug::Level debugLevel = Debug::Level::NONE;

    ApiType apiType = (ApiType)(-1);
    std::string apiPath;
    std::string apiHostname;

    std::string username;
    std::string password;
    bool forceSession = false;

    std::string sessionid;
    std::string sessionkey;
    
    RootType mountRootType = RootType::SUPERROOT;
    std::string mountItemID;
};

#endif