#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include <map>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "Config.hpp"
#include "Utilities.hpp"

/** Manages communication with the backend API */
class Backend
{
public:

    /** Base Exception for all backend issues */
    class Exception : public Utilities::Exception { public:
        /** @param message API error message */
        explicit Exception(const std::string& message) :
            Utilities::Exception("Backend Error: "+message) {}; };

    /** Implements the actual external call to the API */
    class Runner
    {
    public:
        /** Indicates an inability to reach the API endpoint */
        class EndpointException : public Exception { public:
            /** @param code HTTP code returned by the server */
            explicit EndpointException(int code) : 
                Exception("Endpoint: Code "+std::to_string(code)) {};
            /** @param message formatted error message if known */
            explicit EndpointException(const std::string& message) :
                Exception("Endpoint: "+message) {}; };

        /** An API file input param */
        struct InputFile
        {
            /** File name */ std::string name;
            /** File data */ std::string data;
        };

        /** A map of input parameter key to string value */
        typedef std::map<std::string, std::string> Params;

        /** A map of input parameter key to input file */
        typedef std::map<std::string, InputFile> Files;

        /** API app-action call parameters */
        struct Input
        {
            /** app name to run */     std::string app;
            /** app action to run */   std::string action;
            /** map of input params */ Params params = {};
            /** map of input files */  Files files = {};
        };

        virtual ~Runner(){ };

        /**
         * Runs an API call and returns the result
         * @param input input params struct
         * @return result string from API
         */
        virtual std::string RunAction(const Input& input) = 0;

        /** Returns true if the backend requires sessions */
        virtual bool RequiresSession() = 0;
    };

    /** Exception indicating that authentication is required */
    class AuthRequiredException : public Exception { public:
        AuthRequiredException() : Exception("Authentication Required") {}; };

    /** Exception indicating there was a JSON format error */
    class JSONErrorException : public Exception { public:
        /** @param error the JSON error message */
        explicit JSONErrorException(const std::string& error) : 
            Exception("JSON Error: "+error) {}; };

    /** Exception indicating that the backend did not return the correct # of bytes */
    class ReadSizeException : public Exception { public:
        /** @param wanted number of bytes expected
         * @param got number of bytes received */
        explicit ReadSizeException(size_t wanted, size_t got) : Exception(
            "Wanted "+std::to_string(wanted)+" bytes, got "+std::to_string(got)) {}; };

    /** Andromeda exception indicating the requested operation is invalid */
    class UnsupportedException : public Exception { public:
        UnsupportedException() : Exception("Invalid Operation") {}; };

    /** Base exception for Andromeda-returned 403 errors */
    class DeniedException : public Exception { public:
        DeniedException() : Exception("Access Denied") {};
        /** @param message message from backend */
        explicit DeniedException(const std::string& message) : Exception(message) {}; };

    /** Base exception for Andromeda-returned 404 errors */
    class NotFoundException : public Exception { public:
        NotFoundException() : Exception("Not Found") {};
        /** @param message message from backend */
        explicit NotFoundException(const std::string& message) : Exception(message) {}; };

    /** Andromeda exception indicating authentication failed */
    class AuthenticationFailedException : public DeniedException { public:
        AuthenticationFailedException() : DeniedException("Authentication Failed") {}; };

    /** Andromeda exception indicating two factor is needed */
    class TwoFactorRequiredException : public DeniedException { public:
        TwoFactorRequiredException() : DeniedException("Two Factor Required") {}; };

    /** Andromeda exception indicating the server or FS are read only */
    class ReadOnlyException : public DeniedException { public:
        /** @param which string describing what is read-only */
        explicit ReadOnlyException(const std::string& which) : DeniedException("Read Only "+which) {}; };

    /** @param runner the Runner to use */
    explicit Backend(Runner& runner);

    virtual ~Backend();

    /** Initializes the backend by loading config */
    void Initialize(const Config::Options& options);

    /** Sets the backend to use the given username w/o a session */
    void SetUsername(const std::string& username) { this->username = username; }

    /** Registers a pre-existing session ID/key for use */
    void PreAuthenticate(const std::string& sessionID, const std::string& sessionKey);

    /** Creates a new backend session and registers for use */
    void Authenticate(const std::string& username, const std::string& password, const std::string& twofactor = "");

    /** 
     * Creates a new backend session if needed, interactively prompting for input as required 
     * @param username username for authentication
     * @param password optional password for authentication, prompt user if blank
     * @param forceSession if true, force creating a session even if not needed
     */
    void AuthInteractive(const std::string& username, std::string password = "", bool forceSession = false);

    /** Closes the existing session */
    void CloseSession();

    /** Throws if a session is not in use */
    void RequireAuthentication() const;

    /**
     * Loads server config
     * @return loaded config as JSON with "core" and "files" keys
     */
    nlohmann::json GetConfigJ();

    /** Gets the server config object */
    const Config& GetConfig() { return this->config; }

    /** Load limits for the current account */
    nlohmann::json GetAccountLimits();

    /**
     * Load folder metadata (with subitems)
     * @param id folder ID (or blank for default)
     */
    nlohmann::json GetFolder(const std::string& id = "");

    /**
     * Load root folder metadata (no subitems)
     * @param id filesystem ID (or blank for default)
     */
    nlohmann::json GetFSRoot(const std::string& id = "");

    /**
     * Load filesystem metadata
     * @param id filesystem ID (or blank for default)
     */
    nlohmann::json GetFilesystem(const std::string& id = "");

    /**
     * Load limits for a filesystem
     * @param id filesystem ID
     */
    nlohmann::json GetFSLimits(const std::string& id);

    /** Loads filesystem list metadata */
    nlohmann::json GetFilesystems();

    /** Loads items owned but in another user's parent */
    nlohmann::json GetAdopted();
    
    /**
     * Creates a new file
     * @param parent parent folder ID
     * @param name name of new file
     */
    nlohmann::json CreateFile(const std::string& parent, const std::string& name);

    /**
     * Creates a new folder
     * @param parent parent folder ID
     * @param name name of new folder
     */
    nlohmann::json CreateFolder(const std::string& parent, const std::string& name);

    /** Deletes a file by ID */
    void DeleteFile(const std::string& id);

    /** Deletes a folder by ID */
    void DeleteFolder(const std::string& id);

    /** 
     * Renames a file
     * @param id file ID
     * @param name new name
     * @param overwrite whether to overwrite existing
     */
    nlohmann::json RenameFile(const std::string& id, const std::string& name, bool overwrite = false);

    /** 
     * Renames a folder
     * @param id folder ID
     * @param name new name
     * @param overwrite whether to overwrite existing
     */
    nlohmann::json RenameFolder(const std::string& id, const std::string& name, bool overwrite = false);

    /** 
     * Moves a file
     * @param id file ID
     * @param parent new parent ID
     * @param overwrite whether to overwrite existing
     */
    nlohmann::json MoveFile(const std::string& id, const std::string& parent, bool overwrite = false);

    /** 
     * Moves a file
     * @param id file ID
     * @param parent new parent ID
     * @param overwrite whether to overwrite existing
     */
    nlohmann::json MoveFolder(const std::string& id, const std::string& parent, bool overwrite = false);

    /**
     * Reads data from a file
     * @param id file ID
     * @param offset offset to read from
     * @param length number of bytes to read
     */
    std::string ReadFile(const std::string& id, const size_t offset, const size_t length);

    /**
     * Writes data to a file
     * @param id file ID
     * @param offset offset to write to
     * @param data file data to write
     */
    nlohmann::json WriteFile(const std::string& id, const size_t offset, const std::string& data);

    /**
     * Truncates a file
     * @param id file ID
     * @param size new file size
     */
    nlohmann::json TruncateFile(const std::string& id, const size_t size);

private:
    
    /** Base exception for Andromeda-returned errors */
    class APIException : public Exception { public:
        APIException(int code, const std::string& message) : 
            Exception("API code:"+std::to_string(code)+" message:"+message) {}; };

    /**
     * Runs an action with authentication details 
     * @see Runner::RunAction()
     */
    std::string RunAction(Runner::Input& input);

    /** Parses and returns standard Andromeda JSON */
    nlohmann::json GetJSON(const std::string& resp);

    /** Returns true if doing memory only */
    bool isMemory() const;

    /** True if we created the session in use */
    bool createdSession = false;

    std::string username;
    std::string accountID;
    std::string sessionID;
    std::string sessionKey;

    size_t reqCount = 0;

    Runner& runner;
    Config config;
    Debug debug;
};

#endif