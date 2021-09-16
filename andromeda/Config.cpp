#include <vector>

#include <nlohmann/json.hpp>

#include "Config.hpp"
#include "Backend.hpp"

Config::Config() : debug("Config",this){ }

void Config::Initialize(Backend& backend)
{
    this->debug << __func__ << "()"; this->debug.Print();

    nlohmann::json data = backend.GetServerConfig();

    try
    {
        int api = data["config"]["api"];

        if (API_VERSION != api) 
            throw APIVersionException(api);

        const nlohmann::json& apps1 = data["config"]["apps"];
        std::vector apps2 { "server", "accounts", "files" };

        for (const std::string& app : apps2)
            if (apps1.find(app) == apps1.end())
                throw AppMissingException(app);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

}