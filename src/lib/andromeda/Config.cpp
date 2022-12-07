#include <vector>

#include <nlohmann/json.hpp>

#include "Config.hpp"
#include "Backend.hpp"

namespace Andromeda {

/*****************************************************/
Config::Config(Backend& backend) : mDebug("Config",this), mBackend(backend) { }

/*****************************************************/
void Config::Initialize()
{
    mDebug << __func__ << "()"; mDebug.Info();

    nlohmann::json config(mBackend.GetConfigJ());

    try
    {
        const int api { config.at("core").at("api").get<int>() };

        if (API_VERSION != api) 
            throw APIVersionException(api);

        const nlohmann::json& apps1 { config.at("core").at("apps") };
        std::vector<const char*> apps2 { "core", "accounts", "files" };

        for (const std::string app : apps2)
            if (apps1.find(app) == apps1.end())
                throw AppMissingException(app);

        config.at("core").at("features").at("read_only").get_to(mReadOnly);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Config::LoadAccountLimits(Backend& backend)
{
    mDebug << __func__ << "()"; mDebug.Info();

    nlohmann::json limits(backend.GetAccountLimits());

    try
    {
        if (limits != nullptr)
        {
            limits.at("features").at("randomwrite").get_to(mRandWrite);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

} // namespace Andromeda
