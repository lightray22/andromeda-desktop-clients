#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include <string>

#include "Backend.hpp"

/** Runs the API locally by invoking it as a process */
class CLIRunner : public Backend::Runner
{
public:

    /** @param apiPath path to the API index.php */
    CLIRunner(const std::string& apiPath) : apiPath(apiPath){ };

    virtual std::string RunAction(
        const std::string& app, const std::string& action, 
        const Backend::Params& params = Backend::Params()) override;

private:
    const std::string apiPath;
};

#endif