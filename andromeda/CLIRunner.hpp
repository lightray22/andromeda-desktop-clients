#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include <string>

#include "Backend.hpp"

class CLIRunner : public Backend::Runner
{
public:
    CLIRunner(const std::string& apiPath) : apiPath(apiPath){ };

    virtual std::string RunAction(
        const std::string& app, const std::string& action, 
        const Backend::Params& params = Backend::Params()) override;

private:
    const std::string apiPath;
};

#endif