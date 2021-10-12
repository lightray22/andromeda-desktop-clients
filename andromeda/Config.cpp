#include <vector>

#include <nlohmann/json.hpp>

#include "Config.hpp"
#include "Backend.hpp"

/*****************************************************/
Config::Config() : debug("Config",this){ }

/*****************************************************/
void Config::Initialize(Backend& backend)
{
    this->debug << __func__ << "()"; this->debug.Info();

    nlohmann::json config(backend.GetConfigJ());

    try
    {
        int api = config.at("server").at("api").get<int>();

        if (API_VERSION != api) 
            throw APIVersionException(api);

        const nlohmann::json& apps1 = config.at("server").at("apps");
        std::vector<const char*> apps2 { "server", "accounts", "files" };

        for (const std::string app : apps2)
            if (apps1.find(app) == apps1.end())
                throw AppMissingException(app);

        this->readOnly = (config.at("server").at("features").at("read_only").get<std::string>() != "off");
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}