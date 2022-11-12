#include <map>
#include <utility>
#include <nlohmann/json.hpp>

#include "FSConfig.hpp"
#include "Backend.hpp"

namespace Andromeda {

typedef std::map<std::string, FSConfig> CacheMap; static CacheMap cache;

/*****************************************************/
const FSConfig& FSConfig::LoadByID(Backend& backend, const std::string& id)
{
    CacheMap::iterator it { cache.find(id) };

    if (it == cache.end())
    {
        it = cache.emplace(std::piecewise_construct, std::forward_as_tuple(id), 
            std::forward_as_tuple(backend.GetFilesystem(id), backend.GetFSLimits(id))).first;
    }

    return it->second;
}

/*****************************************************/
FSConfig::FSConfig(const nlohmann::json& data, const nlohmann::json& lims) :
    debug("FSConfig", this)
{
    if (data.is_null() && lims.is_null()) return;

    try
    {
        if (data.contains("chunksize"))
            data.at("chunksize").get_to(this->chunksize);

        data.at("readonly").get_to(this->readOnly);

        const std::string sttype { data.at("sttype").get<std::string>() };

        if (sttype == "S3")  this->writeMode = WriteMode::NONE;
        if (sttype == "FTP") this->writeMode = WriteMode::APPEND;

        if (this->writeMode >= WriteMode::RANDOM)
        {
            if (lims.contains("features"))
            {
                const nlohmann::json& rw(lims.at("features").at("randomwrite"));

                if (!rw.is_null() && !rw.get<bool>())
                    this->writeMode = WriteMode::APPEND;
            }
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

} // namespace Andromeda
