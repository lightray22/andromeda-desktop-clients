#include <vector>

#include <nlohmann/json.hpp>

#include "Config.hpp"
#include "BackendImpl.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
Config::Config(BackendImpl& backend) : mDebug("Config",this), mBackend(backend) { }

/*****************************************************/
void Config::Initialize()
{
    MDBG_INFO("()");

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
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Config::LoadAccountLimits(BackendImpl& backend)
{
    MDBG_INFO("()");

    nlohmann::json limits(backend.GetAccountLimits());

    try
    {
        if (limits != nullptr)
        {
            limits.at("features").at("randomwrite").get_to(mRandWrite);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

} // namespace Backend
} // namespace Andromeda
