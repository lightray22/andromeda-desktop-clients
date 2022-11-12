#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include <chrono>
#include <string>
#include <system_error>

#include "Backend.hpp"
#include "Utilities.hpp"

namespace Andromeda {

/** Runs the API locally by invoking it as a process */
class CLIRunner : public Backend::Runner
{
public:

    /** Exception indicating the CLI runner had an error */
    class Exception : public EndpointException { public: 
        /** @param msg error message string */
        explicit Exception(const std::string& msg) : 
            EndpointException("Subprocess Error: "+msg) {} };

    /** @param apiPath path to the API index.php */
    explicit CLIRunner(const std::string& apiPath);

    virtual std::string RunAction(const Input& input) override;

    virtual bool RequiresSession() override { return false; }

private:

    Debug debug;

    std::string apiPath;

    const std::chrono::seconds timeout { std::chrono::seconds(120) };
};

} // namespace Andromeda

#endif