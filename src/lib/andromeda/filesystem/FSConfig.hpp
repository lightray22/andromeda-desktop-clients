#ifndef LIBA2_FSCONFIG_H_
#define LIBA2_FSCONFIG_H_

#include <string>
#include "nlohmann/json_fwd.hpp"

#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {

/** 
 * Contains backend filesystem config 
 * THREAD SAFE (INTERNAL LOCKS)
 */
class FSConfig
{
public:
    virtual ~FSConfig() = default;

    /**
     * Load from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of filesystem to load
     * @throws BackendException on any backend error
     */
    static const FSConfig& LoadByID(Backend::BackendImpl& backend, const std::string& id);

    /** 
     * Construct with JSON data
     * @param data json data from backend
     * @param lims json limit data from backend
     * @throws BackendImpl::JSONErrorException on JSON errors
     */
    FSConfig(const nlohmann::json& data, const nlohmann::json& lims);

    /** Returns the filesystem chunk size or 0 for none */
    [[nodiscard]] size_t GetChunkSize() const { return mChunksize; }

    /** Returns true if the filesystem is read-only */
    [[nodiscard]] bool isReadOnly() const { return mReadOnly; }

    /** The modify mode supported by the filesystem */
    enum class WriteMode : uint8_t { UPLOAD, APPEND, RANDOM };

    /** Returns whether append/random write is allowed */
    [[nodiscard]] WriteMode GetWriteMode() const { return mWriteMode; }

private:

    /** Chunk size preferred by the backend */
    size_t mChunksize { 0 };
    /** True if the filesystem is read-only */
    bool mReadOnly { false };
    /** WriteMode supported by the filesystem */
    WriteMode mWriteMode { WriteMode::RANDOM };

    mutable Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FSCONFIG_H_
