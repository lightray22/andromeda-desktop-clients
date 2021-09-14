
#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include <string>
#include "Runner.hpp"
#include "Config.hpp"

#include <nlohmann/json_fwd.hpp>

class Backend
{
public:

    Backend(Runner& runner);

    void Authenticate(const std::string& username);

private:

    nlohmann::json DecodeResponse(const std::string& response);

    Runner& runner;
    Config config;
};

#endif