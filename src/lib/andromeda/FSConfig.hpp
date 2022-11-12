#ifndef LIBA2_FSCONFIG_H_
#define LIBA2_FSCONFIG_H_

#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

namespace Andromeda {

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
     * @param data json data from backend
     * @param lims json limit data from backend
     */
    FSConfig(const nlohmann::json& data, const nlohmann::json& lims);

    /** Returns the filesystem chunk size or 0 for none */
    size_t GetChunkSize() const { return this->chunksize; }

    /** Returns true if the filesystem is read-only */
    bool isReadOnly() const { return this->readOnly; }

    /** The overwrite mode supported by the filesystem */
    enum class WriteMode { NONE, APPEND, RANDOM };

    /** Returns whether append/random write is allowed */
    WriteMode GetWriteMode() const { return this->writeMode; }

private:

    size_t chunksize { 0 };

    bool readOnly { false };

    WriteMode writeMode { WriteMode::RANDOM };

    Debug debug;
};

} // namespace Andromeda

#endif