#ifndef LIBA2_CONFIG_H_
#define LIBA2_CONFIG_H_

#include <atomic>
#include <chrono>
#include "nlohmann/json_fwd.hpp"

#include "BackendException.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Backend {

class BackendImpl;

/** 
 * Checks and stores backend config 
 * THREAD SAFE (INTERNAL LOCKS)
 */
class Config
{
public:

    /**
     * Loads config from the backend
     * @throws BackendException for backend issues
     */
    explicit Config(BackendImpl& backend);
    
    /** The Major API version this client works with */
    static constexpr int API_VERSION { 2 };

    /** Base exception for Config exceptions */
    class Exception : public BackendException { public:
        /** @param message error message string */
        explicit Exception(const std::string& message) :
            BackendException("Config Error: "+message){}; };

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

    /** 
     * Adds account-specific limits
     * @throws BackendException for backend issues
     */
    void LoadAccountLimits(BackendImpl& backend);

    /** Returns true if the backend is read-only */
    [[nodiscard]] bool isReadOnly() const { return mReadOnly.load(); }

    /** Returns true if random write is allowed */
    [[nodiscard]] bool canRandWrite() const { return mRandWrite.load(); }

    /** Returns the max # of bytes allowed in an upload or 0 for no limit */
    [[nodiscard]] size_t GetUploadMaxBytes() const { return mUploadMaxBytes.load(); }

    /** Sets the max upload bytes in case we discover it is lower than thought */
    void SetUploadMaxBytes(const size_t newMax) { mUploadMaxBytes.store(newMax); };

private:
    mutable Debug mDebug;
    BackendImpl& mBackend;

    std::atomic<bool> mReadOnly { false };
    std::atomic<bool> mRandWrite { true };

    std::atomic<size_t> mUploadMaxBytes { 0 };
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_CONFIG_H_
