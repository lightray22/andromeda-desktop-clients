#ifndef LIBA2_FSCONFIG_H_
#define LIBA2_FSCONFIG_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

class Backend;

/** Contains filesystem config */
class FSConfig
{
public:
    virtual ~FSConfig(){};

    /**
     * Load from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of filesystem to load
     */
    static const FSConfig& LoadByID(Backend& backend, const std::string& id);

    /** 
     * Construct with JSON data
     * @param backend reference to backend
     * @param data json data from backend
     */
    FSConfig(Backend& backend, const nlohmann::json& data);

    /** Returns the filesystem chunk size or 0 for none */
    size_t GetChunkSize() const { return this->chunksize; }

private:

    size_t chunksize = 0;

    Debug debug;
};

#endif