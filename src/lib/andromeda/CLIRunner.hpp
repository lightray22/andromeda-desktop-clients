#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include <chrono>
#include <string>
#include <system_error>

#include "Backend.hpp"
#include "Utilities.hpp"

/** Runs the API locally by invoking it as a process */
class CLIRunner : public Backend::Runner
{
public:

    /** Exception indicating the CLI runner had an error */
    class Exception : public Backend::Exception { public: 
        Exception(std::error_code code) :
            Backend::Exception("Subprocess Error: "+code.message()) {}
        Exception(const std::string& msg) : 
            Backend::Exception("Subprocess Code: "+msg) {} };

    /** @param apiPath path to the API index.php */
    CLIRunner(const std::string& apiPath);

    virtual std::string RunAction(const Input& input) override;

private:

    Debug debug;

    std::string apiPath;

    const std::chrono::seconds timeout = std::chrono::seconds(120);
};

#endif