#ifndef LIBA2_BASERUNNER_H_
#define LIBA2_BASERUNNER_H_

#include <map>
#include <string>

#include "BackendImpl.hpp"
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
    class EndpointException : public BackendImpl::Exception { public:
        /** @param code HTTP code returned by the server */
        explicit EndpointException(int code) : 
            Exception("Endpoint: Code "+std::to_string(code)) {};
        /** @param message formatted error message if known */
        explicit EndpointException(const std::string& message) :
            Exception("Endpoint: "+message) {}; };

    virtual ~BaseRunner(){ }; // for unique_ptr

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
     * @param input input params struct with file streams
     * @return result string from API
     */
    virtual std::string RunAction(const RunnerInput_StreamIn& input) = 0;
    
    /**
     * Runs an API call and streams the result
     * @param input input params struct with streamer
     */
    virtual void RunAction(const RunnerInput_StreamOut& input) = 0;

    /** Returns true if the backend requires sessions */
    virtual bool RequiresSession() = 0;
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_BASERUNNER_H_
