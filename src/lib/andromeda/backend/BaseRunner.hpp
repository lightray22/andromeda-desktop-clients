#ifndef LIBA2_BASERUNNER_H_
#define LIBA2_BASERUNNER_H_

#include <map>
#include <memory>
#include <string>

#include "andromeda/BaseException.hpp"

namespace Andromeda {
namespace Backend {

struct RunnerInput;
struct RunnerInput_FilesIn;
struct RunnerInput_StreamIn;
struct RunnerInput_StreamOut;

/** Implements the actual external call to the API */
class BaseRunner
{
public:
    /** Indicates an inability to reach the API endpoint */
    class EndpointException : public BaseException { public:
        /** @param code HTTP code returned by the server */
        explicit EndpointException(int code) : 
            BaseException("Endpoint Error: Code "+std::to_string(code)) {};
        /** @param message formatted error message if known */
        explicit EndpointException(const std::string& message) :
            BaseException("Endpoint Error: "+message) {}; };

    virtual ~BaseRunner(){ }; // for unique_ptr

    /** Copies to a new runner with a new backend channel, but the same config */
    virtual std::unique_ptr<BaseRunner> Clone() = 0;

    /** Returns the remote hostname of the runner */
    virtual std::string GetHostname() const = 0;

    /**
     * Runs an API call and returns the result
     * @param input input params struct
     * @return result string from API
     */
    virtual std::string RunAction(const RunnerInput& input) = 0;

    /**
     * Runs an API call and returns the result
     * @param input input params struct with files
     * @return result string from API
     */
    virtual std::string RunAction(const RunnerInput_FilesIn& input) = 0;
    
    /**
     * Runs an API call and returns the result
     * MUST NOT call another action within the callback!
     * @param input input params struct with file streams
     * @return result string from API
     */
    virtual std::string RunAction(const RunnerInput_StreamIn& input) = 0;
    
    /**
     * Runs an API call and streams the result
     * MUST NOT call another action within the callback!
     * @param input input params struct with streamer
     */
    virtual void RunAction(const RunnerInput_StreamOut& input) = 0;

    /** Returns true if the backend requires sessions */
    virtual bool RequiresSession() const = 0;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_BASERUNNER_H_
