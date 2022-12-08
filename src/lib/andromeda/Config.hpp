#ifndef LIBA2_CONFIG_H_
#define LIBA2_CONFIG_H_

#include <chrono>
#include <nlohmann/json_fwd.hpp>

#include "BaseException.hpp"
#include "Debug.hpp"

namespace Andromeda {

class Backend;

/** Checks and stores backend config */
class Config
{
public:
    explicit Config(Backend& backend);
    
    /** The Major API version this client works with */
    static constexpr int API_VERSION { 2 };

    /** Base exception for Config exceptions */
    class Exception : public BaseException { public:
        /** @param message error message string */
        explicit Exception(const std::string& message) :
            BaseException("Config Error: "+message){}; };

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

    /** Loads config from the given backend */
    void Initialize();

    /** Adds account-specific limits */
    void LoadAccountLimits(Backend& backend);

    /** Returns true if the backend is read-only */
    bool isReadOnly() const { return mReadOnly; }

    /** Returns true if random write is allowed */
    bool canRandWrite() const { return mRandWrite; }

    /** Returns the max # of bytes allowed in an upload */
    uint64_t GetUploadMaxBytes() const { return mUploadMaxBytes; }

    /** Returns the max # of files allowed in an upload */
    uint64_t GetUploadMaxFiles() const { return mUploadMaxFiles; }

private:
    Debug mDebug;
    Backend& mBackend;

    bool mReadOnly { false };
    bool mRandWrite { true };

    uint64_t mUploadMaxBytes { 0 };
    uint64_t mUploadMaxFiles { 0 };
};

} // namespace Andromeda

#endif
