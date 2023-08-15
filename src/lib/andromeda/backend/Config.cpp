#include <vector>

#include "nlohmann/json.hpp"

#include "Config.hpp"
#include "BackendImpl.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
Config::Config(BackendImpl& backend) : 
    mDebug(__func__,this), mBackend(backend)
{
    MDBG_INFO("()");

    nlohmann::json coreConfig(mBackend.GetCoreConfigJ());
    nlohmann::json filesConfig(mBackend.GetFilesConfigJ());

    try
    {
        const int api { coreConfig.at("api").get<int>() };

        if (API_VERSION != api) 
            throw APIVersionException(api);

        const nlohmann::json& appsHave { coreConfig.at("apps") };
        static const std::vector<const char*> appsReq { "core", "accounts", "files" };

        for (const std::string appReq : appsReq) // NOT string&
            if (appsHave.find(appReq) == appsHave.end())
                throw AppMissingException(appReq);

        // can't get_to() with std::atomic
        mReadOnly = coreConfig.at("features").at("read_only").get<bool>();

        const nlohmann::json& maxbytes { filesConfig.at("upload_maxbytes") };
        if (!maxbytes.is_null()) mUploadMaxBytes = maxbytes.get<size_t>();

        // TODO the server also has upload_maxsize... what is that?
        // TODO the server also has crchunksize... what is that?
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
            mRandWrite = limits.at("features").at("randomwrite").get<bool>();
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

} // namespace Backend
} // namespace Andromeda
