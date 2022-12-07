#ifndef A2FUSE_OPTIONS_H_
#define A2FUSE_OPTIONS_H_

#include <filesystem>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>

#include "andromeda-fuse/FuseAdapter.hpp"

#include "andromeda/BackendOptions.hpp"
#include "andromeda/HTTPRunnerOptions.hpp"
#include "andromeda/Utilities.hpp"

/** Manages command line options and config */
class Options
{
public:

    /** Base class for all Options errors */
    class Exception : public Andromeda::Utilities::Exception { 
        using Andromeda::Utilities::Exception::Exception; };

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
        public: explicit BadFlagException(const std::string& flag) : 
            Exception("Unknown Flag: "+flag) {} };

    /** Exception indicating the given option is unknown */
    class BadOptionException : public Exception { 
        /** @param option the unknown option */
        public: explicit BadOptionException(const std::string& option) : 
            Exception("Unknown Option: "+option) {} };

    /** Exception indicating the given option has a bad value */
    class BadValueException : public Exception {
        /** @param option the option name */
        public: explicit BadValueException(const std::string& option) :
            Exception("Bad Option Value: "+option) {} };

    /** Exception indicating the given option is required but not present */
    class MissingOptionException : public Exception {
        /** @param option the option name */
        public: explicit MissingOptionException(const std::string& option) :
            Exception("Missing Option: "+option) {} };

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /**
     * @param configOptions Config options ref to fill
     * @param httpOptions HTTPRunner options ref to fill (if applicable)
     * @param fuseOptions FUSE options ref to fill
     */
    Options(Andromeda::BackendOptions& configOptions, 
            Andromeda::HTTPRunnerOptions& httpOptions, 
            AndromedaFuse::FuseAdapter::Options& fuseOptions);

    /** Parses command line arguments from main */
    void ParseArgs(int argc, char** argv);

    /** Parses arguments from a config file */
    void ParseFile(const std::filesystem::path& path);

    /** Makes sure all required args were provided */
    void Validate();

    /** Returns the desired debug level */
    Andromeda::Debug::Level GetDebugLevel() const { return mDebugLevel; }

    /** Backend connection type */
    enum class ApiType
    {
        API_URL,
        API_PATH
    };

    /** Returns the specified API type */
    ApiType GetApiType() const { return mApiType; }    

    /** Returns the path to the API endpoint */
    std::string GetApiPath() const { return mApiPath; }

    /** Returns true if a username is specified */
    bool HasUsername() const { return !mUsername.empty(); }

    /** Returns the specified username */
    std::string GetUsername() const { return mUsername; }

    /** Returns true if a password is specified */
    bool HasPassword() const { return !mPassword.empty(); }

    /** Returns the specified password */
    std::string GetPassword() const { return mPassword; }

    /** Returns true if a session ID was provided */
    bool HasSession() const { return !mSessionid.empty(); }

    /** Returns the specified session ID */
    std::string GetSessionID() const { return mSessionid; }

    /** Returns the specified session key */
    std::string GetSessionKey() const { return mSessionkey; }

    /** Returns true if using a session is forced */
    bool GetForceSession() const { return mForceSession; }

    /** Folder types that can be mounted as root */
    enum class RootType
    {
        SUPERROOT,
        FILESYSTEM,
        FOLDER
    };

    /** Returns the specified mount item type */
    RootType GetMountRootType() const { return mMountRootType; }

    /** Returns the specified mount item ID */
    std::string GetMountItemID() const { return mMountItemID; }

private:

    /** Load config from the given flags and options */
    void LoadFrom(const Andromeda::Utilities::Flags& flags, const Andromeda::Utilities::Options options);

    Andromeda::BackendOptions& mConfigOptions;
    Andromeda::HTTPRunnerOptions& mHttpOptions;
    AndromedaFuse::FuseAdapter::Options& mFuseOptions;

    Andromeda::Debug::Level mDebugLevel { Andromeda::Debug::Level::NONE };

    ApiType mApiType { static_cast<ApiType>(-1) };
    std::string mApiPath;

    std::string mUsername;
    std::string mPassword;
    bool mForceSession { false };

    std::string mSessionid;
    std::string mSessionkey;
    
    RootType mMountRootType { RootType::SUPERROOT };
    std::string mMountItemID;
};

#endif // A2FUSE_OPTIONS_H_
