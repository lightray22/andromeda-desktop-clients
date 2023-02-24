#ifndef LIBA2_BACKENDIMPL_H_
#define LIBA2_BACKENDIMPL_H_

#include <atomic>
#include <functional>
#include <map>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "Config.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace Filesystem { namespace Filedata { class CacheManager; } }

namespace Backend {

class BaseRunner;
struct RunnerInput;
struct RunnerInput_FilesIn;
struct RunnerInput_StreamIn;

/** Manages communication with the backend API */
class BackendImpl
{
public:

    /** Base Exception for all backend issues */
    class Exception : public BaseException { public:
        /** @param message API error message */
        explicit Exception(const std::string& message) :
            BaseException("Backend Error: "+message) {}; };

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

    /** Exception indicating we set the backend as read-only */
    class ReadOnlyException : public Exception { public:
        explicit ReadOnlyException() : Exception("Read Only Backend") {}; };

    /** Base exception for Andromeda-returned errors */
    class APIException : public Exception { public:
        using Exception::Exception; // string constructor
        APIException(int code, const std::string& message) : 
            Exception("API code:"+std::to_string(code)+" message:"+message) {}; };

    /** Andromeda exception indicating the requested operation is invalid */
    class UnsupportedException : public APIException { public:
        UnsupportedException() : APIException("Invalid Operation") {}; };

    /** Base exception for Andromeda-returned 403 errors */
    class DeniedException : public APIException { public:
        DeniedException() : APIException("Access Denied") {};
        /** @param message message from backend */
        explicit DeniedException(const std::string& message) : APIException(message) {}; };

    /** Andromeda exception indicating authentication failed */
    class AuthenticationFailedException : public DeniedException { public:
        AuthenticationFailedException() : DeniedException("Authentication Failed") {}; };

    /** Andromeda exception indicating two factor is needed */
    class TwoFactorRequiredException : public DeniedException { public:
        TwoFactorRequiredException() : DeniedException("Two Factor Required") {}; };

    /** Andromeda exception indicating the server or FS are read only */
    class ReadOnlyFSException : public DeniedException { public:
        /** @param which string describing what is read-only */
        explicit ReadOnlyFSException(const std::string& which) : DeniedException("Read Only "+which) {}; };

    /** Base exception for Andromeda-returned 404 errors */
    class NotFoundException : public APIException { public:
        NotFoundException() : APIException("Not Found") {};
        /** @param message message from backend */
        explicit NotFoundException(const std::string& message) : APIException(message) {}; };

    /**
     * @param options configuration options
     * @param runner the BaseRunner to use 
     */
    BackendImpl(const Andromeda::ConfigOptions& options, BaseRunner& runner);

    virtual ~BackendImpl();

    /** Initializes the backend by loading config */
    void Initialize();

    /** Gets the server config object */
    const Config& GetConfig() { return mConfig; }

    /** Returns the backend options in use */
    const Andromeda::ConfigOptions& GetOptions() const { return mOptions; }

    /** Returns the cache manager if set or nullptr */
    Filesystem::Filedata::CacheManager* GetCacheManager() const { return mCacheMgr; }

    /** Sets the cache manager to use (or nullptr) */
    void SetCacheManager(Filesystem::Filedata::CacheManager* cacheMgr) { mCacheMgr = cacheMgr; }

    /** Returns true if doing memory only */
    bool isMemory() const;

    /** Returns true if the backend is read-only */
    bool isReadOnly() const;

    /** 
     * Return the hostname_username ID string 
     * @param human if true make it human-pretty
     */
    std::string GetName(bool human) const;

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

    /** 
     * Throws if a session is not in use 
     * @throws AuthRequiredException if no session
     */
    void RequireAuthentication() const;

    /*****************************************************/
    // ---- Actual backend functions below here ---- //

    /**
     * Loads server config
     * @return loaded config as JSON with "core" and "files" keys
     */
    nlohmann::json GetConfigJ();

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
     * @param overwrite whether to overwrite existing
     */
    nlohmann::json CreateFile(const std::string& parent, const std::string& name, bool overwrite = false);

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
    std::string ReadFile(const std::string& id, const uint64_t offset, const size_t length);

    /** 
     * A function that supplies a buffer to read output data out of
     * @param offset offset of the current output data 
     * @param data pointer to buffer containing data
     * @param length length of the data buffer
     */
    typedef std::function<void(const size_t offset, const char* buf, const size_t length)> ReadFunc;

    /**
     * Streams data from a file
     * @param id file ID
     * @param offset offset to read from
     * @param length number of bytes to read
     * @param func data handler function
     */
    void ReadFile(const std::string& id, const uint64_t offset, const size_t length, ReadFunc func);

    /**
     * Writes data to a file
     * @param id file ID
     * @param offset offset to write to
     * @param data file data to write
     */
    nlohmann::json WriteFile(const std::string& id, const uint64_t offset, const std::string& data);
    
    /**
     * Creates a new file with data
     * @param parent parent folder ID
     * @param name name of new file
     * @param data file data to write
     * @param overwrite whether to overwrite existing
     */
    nlohmann::json UploadFile(const std::string& parent, const std::string& name, const std::string& data, bool overwrite = false);

    /**
     * Truncates a file
     * @param id file ID
     * @param size new file size
     */
    nlohmann::json TruncateFile(const std::string& id, const uint64_t size);

private:
    
    /** Augment input with authentication details */
    RunnerInput& FinalizeInput(RunnerInput& input);

    /** Prints a RunnerInput to the given stream */
    void PrintInput(RunnerInput& input, std::ostream& str, const std::string& myfname);
    /** Prints a RunnerInput_FilesIn to the given stream */
    void PrintInput(RunnerInput_FilesIn& input, std::ostream& str, const std::string& myfname);
    /** Prints a RunnerInput_StreamIn to the given stream */
    void PrintInput(RunnerInput_StreamIn& input, std::ostream& str, const std::string& myfname);

    /** Parses and returns standard Andromeda JSON */
    nlohmann::json GetJSON(const std::string& resp);

    /** True if we created the session in use */
    bool mCreatedSession { false };

    std::string mUsername;
    std::string mAccountID;
    std::string mSessionID;
    std::string mSessionKey;
    
    // global backend request counter
    static std::atomic<uint64_t> sReqCount;

    ConfigOptions mOptions;
    BaseRunner& mRunner;

    Filesystem::Filedata::CacheManager* mCacheMgr { nullptr };

    Config mConfig;
    Debug mDebug;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_BACKENDIMPL_H_
