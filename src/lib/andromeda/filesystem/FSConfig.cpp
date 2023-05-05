#include <mutex>
#include <unordered_map>
#include <utility>
#include <nlohmann/json.hpp>

#include "FSConfig.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {

typedef std::unordered_map<std::string, FSConfig> CacheMap; 
static CacheMap sCache; static std::mutex sCacheMutex;

/*****************************************************/
const FSConfig& FSConfig::LoadByID(BackendImpl& backend, const std::string& id)
{
    std::lock_guard<decltype(sCacheMutex)> llock(sCacheMutex);

    CacheMap::iterator it { sCache.find(id) };

    if (it == sCache.end())
    {
        it = sCache.emplace(std::piecewise_construct, std::forward_as_tuple(id), 
            std::forward_as_tuple(backend.GetFilesystem(id), backend.GetFSLimits(id))).first;
    }

    return it->second;
}

/*****************************************************/
FSConfig::FSConfig(const nlohmann::json& data, const nlohmann::json& lims) :
    mDebug(__func__, this)
{
    if (data.is_null() && lims.is_null()) return;

    try
    {
        if (data.contains("chunksize"))
            data.at("chunksize").get_to(mChunksize);

        data.at("readonly").get_to(mReadOnly);

        const std::string sttype { data.at("sttype").get<std::string>() };

        if (sttype == "S3")  mWriteMode = WriteMode::UPLOAD;
        if (sttype == "FTP") mWriteMode = WriteMode::APPEND;

        if (mWriteMode >= WriteMode::RANDOM)
        {
            if (lims.contains("features"))
            {
                const nlohmann::json& rw(lims.at("features").at("randomwrite"));

                if (!rw.is_null() && !rw.get<bool>())
                    mWriteMode = WriteMode::APPEND;
            }
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

} // namespace Filesystem
} // namespace Andromeda
