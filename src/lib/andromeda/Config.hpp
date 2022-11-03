#ifndef LIBA2_CONFIG_H_
#define LIBA2_CONFIG_H_

#include <chrono>
#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

class Backend;

/** Checks and stores backend config */
class Config
{
public:
    Config();
    
    /** The Major API version this client works with */
    static constexpr int API_VERSION = 2;

    /** Base exception for Config exceptions */
    class Exception : public Utilities::Exception { public:
        /** @param message error message string */
        explicit Exception(const std::string& message) :
            Utilities::Exception("Config Error: "+message){}; };

    /** Exception indicating the API version is not supported */
    class APIVersionException : public Exception { public:
        /** @param version the backend's version */
        explicit APIVersionException(int version) : 
            Exception("API Version is "+std::to_string(version)+
                    ", require "+std::to_string(API_VERSION)){}; };

    /** Exception indicating a required app is missing */
    class AppMissingException : public Exception { public:
        /** @param appname name of the app that is missing */
        explicit AppMissingException(const std::string& appname) :
            Exception("Missing app: "+appname){}; };

    /** Client-based backend options */
    struct Options
    {
        /** Client cache modes (debug) */
        enum class CacheType
        {
            /** read/write directly to server */  NONE,
            /** never contact server (testing) */ MEMORY,
            /** normal read/write in pages */     NORMAL
        };

        /** The client cache type (debug) */
        CacheType cacheType = CacheType::NORMAL;
        /** The file data page size */
        size_t pageSize = 1024*1024; // 1M
        /** Whether we are in read-only mode */
        bool readOnly = false;
        /** The time period to use for refreshing API data */
        std::chrono::seconds refreshTime = std::chrono::seconds(15);
    };

    /** Sets config from the given backend and options */
    void Initialize(Backend& backend, const Options& options);

    /** Adds account-specific limits */
    void LoadAccountLimits(Backend& backend);

    /** Gets the configured backend options */
    const Options& GetOptions() const { return this->options; }

    /** Returns true if the backend is read-only */
    bool isReadOnly() const { return this->readOnly; }

    /** Returns true if random write is allowed */
    bool canRandWrite() const { return this->randWrite; }

    /** Returns the max # of bytes allowed in an upload */
    unsigned int GetUploadMaxBytes() const { return this->uploadMaxBytes; }

    /** Returns the max # of files allowed in an upload */
    unsigned int GetUploadMaxFiles() const { return this->uploadMaxFiles; }

private:
    Debug debug;

    Options options;

    bool readOnly = false;
    bool randWrite = true;

    unsigned int uploadMaxBytes = 0;
    unsigned int uploadMaxFiles = 0;
};

#endif