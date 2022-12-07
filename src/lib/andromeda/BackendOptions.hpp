#ifndef LIBA2_BACKENDOPTIONS_H_
#define LIBA2_BACKENDOPTIONS_H_

#include <chrono>

namespace Andromeda {

/** Client-based backend options */
struct BackendOptions
{
    /** Client cache modes (debug) */
    enum class CacheType
    {
        /** read/write directly to server */  NONE,
        /** never contact server (testing) */ MEMORY,
        /** normal read/write in pages */     NORMAL
    };

    /** The client cache type (debug) */
    CacheType cacheType { CacheType::NORMAL };
    /** The file data page size */
    size_t pageSize { 1024*1024 }; // 1M
    /** Whether we are in read-only mode */
    bool readOnly { false };
    /** The time period to use for refreshing API data */
    std::chrono::seconds refreshTime { std::chrono::seconds(15) };
};

} // namespace Andromeda

#endif // LIBA2_BACKENDOPTIONS_H_