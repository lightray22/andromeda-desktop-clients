#include <map>
#include <utility>
#include <nlohmann/json.hpp>

#include "FSConfig.hpp"
#include "Backend.hpp"

typedef std::map<std::string, FSConfig> CacheMap; static CacheMap cache;

/*****************************************************/
const FSConfig& FSConfig::LoadByID(Backend& backend, const std::string& id)
{
    CacheMap::iterator it = cache.find(id);

    if (it == cache.end())
    {
        it = cache.emplace(std::piecewise_construct, std::forward_as_tuple(id), 
            std::forward_as_tuple(backend, backend.GetFilesystem(id))).first;
    }

    return it->second;
}

/*****************************************************/
FSConfig::FSConfig(Backend& backend, const nlohmann::json& data) :
    debug("FSConfig", this)
{
    try
    {
        if (data.contains("chunksize"))
            data.at("chunksize").get_to(this->chunksize);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}