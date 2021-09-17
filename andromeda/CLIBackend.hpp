#ifndef LIBA2_CLIBACKEND_H_
#define LIBA2_CLIBACKEND_H_

#include <string>

#include "Backend.hpp"

class CLIBackend : public Backend
{
public:
    CLIBackend(const std::string& apiPath) : apiPath(apiPath){ };

    virtual std::string RunAction(const std::string& app, const std::string& action, const Params& params = Params()) override;

private:
    const std::string apiPath;
};

#endif