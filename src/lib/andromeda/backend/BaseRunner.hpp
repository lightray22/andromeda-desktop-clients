#ifndef LIBA2_BASERUNNER_H_
#define LIBA2_BASERUNNER_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>

#include "BackendException.hpp"
#include "andromeda/common.hpp"

namespace Andromeda {
namespace Backend {

struct RunnerInput;
struct RunnerInput_FilesIn;
struct RunnerInput_StreamIn;
struct RunnerInput_StreamOut;

/** 
 * Implements the actual external call to the API 
 * NOT THREAD SAFE (use a RunnerPool)
 */
class BaseRunner
{
public:
    /** Indicates an inability to reach the API endpoint */
    class EndpointException : public BackendException { public:
        /** @param code HTTP code returned by the server */
        explicit EndpointException(int code) : 
            BackendException("Endpoint Error: Code "+std::to_string(code)) {};
        /** @param message formatted error message if known */
        explicit EndpointException(const std::string& message) :
            BackendException("Endpoint Error: "+message) {}; };

    BaseRunner() = default;
    virtual ~BaseRunner() = default;
    DELETE_COPY(BaseRunner)
    DELETE_MOVE(BaseRunner)

    /** Copies to a new runner with a new backend channel, but the same config */
    [[nodiscard]] virtual std::unique_ptr<BaseRunner> Clone() const = 0;

    /** Returns the remote hostname of the runner */
    [[nodiscard]] virtual std::string GetHostname() const = 0;

    /** Allows automatic retry on request failure */
    inline void EnableRetry(bool enable = true) { mCanRetry.store(enable); }

    /** Returns whether retry is enabled or disabled */
    [[nodiscard]] inline bool GetCanRetry() const { return mCanRetry.load(); }

    /**
     * Runs an API call and returns the result
     * @param input input params struct
     * @return result string from API
     * @throws EndpointException on any failure
     */
    virtual std::string RunAction_Read(const RunnerInput& input) = 0;

    /**
     * Runs an API call and returns the result
     * @param input input params struct
     * @return result string from API
     * @throws EndpointException on any failure
     */
    virtual std::string RunAction_Write(const RunnerInput& input) = 0;

    /**
     * Runs an API call and returns the result
     * @param input input params struct with files
     * @return result string from API
     * @throws EndpointException on any failure
     */
    virtual std::string RunAction_FilesIn(const RunnerInput_FilesIn& input) = 0;
    
    /**
     * Runs an API call and returns the result
     * MUST NOT call another action within the callback!
     * @param input input params struct with file streams
     * @return result string from API
     * @throws EndpointException on any failure
     */
    virtual std::string RunAction_StreamIn(const RunnerInput_StreamIn& input) = 0;
    
    /**
     * Runs an API call and streams the result
     * MUST NOT call another action within the callback!
     * @param input input params struct with streamer
     * @throws EndpointException on any failure
     */
    virtual void RunAction_StreamOut(const RunnerInput_StreamOut& input) = 0;

    /** Returns true if the backend requires sessions */
    [[nodiscard]] virtual bool RequiresSession() const = 0;
    
private:

    std::atomic<bool> mCanRetry { false };
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_BASERUNNER_H_
