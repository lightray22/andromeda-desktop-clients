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
    
    static constexpr int API_VERSION = 2;

    /** Base exception for Config exceptions */
    class Exception : public Utilities::Exception { public:
        Exception(const std::string& message) :
            Utilities::Exception("Config Error: "+message){}; };

    /** Exception indicating the API version is not supported */
    class APIVersionException : public Exception { public:
        APIVersionException(int version) : 
            Exception("API Version is "+std::to_string(version)+
                    ", require "+std::to_string(API_VERSION)){}; };

    /** Exception indicating a required app is missing */
    class AppMissingException : public Exception { public:
        AppMissingException(const std::string& appname) :
            Exception("Missing app: "+appname){}; };

    /** Client-based backend options */
    struct Options
    {
        enum class CacheType
        {
            NONE,   // read/write directly to server
            MEMORY, // never contact server (testing)
            NORMAL  // normal read/write in pages
        };

        CacheType cacheType = CacheType::NORMAL;

        size_t pageSize = 1024*1024; // 1M

        std::chrono::seconds folderRefresh = std::chrono::seconds(15);
    };

    /** Sets config from the given backend and options */
    void Initialize(Backend& backend, const Options& options);

    /** Gets the configured backend options */
    const Options& GetOptions() const { return this->options; }

    /** Returns true if the backend is read-only */
    bool isReadOnly() const { return this->readOnly; }

    /** Returns the max # of bytes allowed in an upload */
    unsigned int GetUploadMaxBytes() const { return this->uploadMaxBytes; }

    /** Returns the max # of files allowed in an upload */
    unsigned int GetUploadMaxFiles() const { return this->uploadMaxFiles; }

private:
    Debug debug;

    Options options;

    bool readOnly;

    unsigned int uploadMaxBytes = 0;
    unsigned int uploadMaxFiles = 0;
};

#endif