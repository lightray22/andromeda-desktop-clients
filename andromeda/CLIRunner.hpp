#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include "Runner.hpp"
#include <string>

class CLIRunner : public Runner
{
public:
    CLIRunner(const std::string& apiPath) : apiPath(apiPath){ };

    virtual std::string RunAction(const std::string& app, const std::string& action) override;

private:
    const std::string apiPath;
};

#endif