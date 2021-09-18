#include <vector>

#include <nlohmann/json.hpp>

#include "Config.hpp"
#include "Backend.hpp"

Config::Config() : debug("Config",this){ }

void Config::Initialize(Backend& backend)
{
    this->debug << __func__ << "()"; this->debug.Print();

    nlohmann::json config(backend.GetConfig());

    try
    {
        int api = config["server"]["api"];

        if (API_VERSION != api) 
            throw APIVersionException(api);

        const nlohmann::json& apps1 = config["server"]["apps"];
        std::vector apps2 { "server", "accounts", "files" };

        for (const std::string& app : apps2)
            if (apps1.find(app) == apps1.end())
                throw AppMissingException(app);

        this->readOnly = config["server"]["features"]["read_only"] != "off";
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}