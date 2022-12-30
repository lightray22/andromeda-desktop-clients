#ifndef LIBA2_CONFIGOPTIONS_H_
#define LIBA2_CONFIGOPTIONS_H_

#include <chrono>
#include <string>

namespace Andromeda {
namespace Backend {

/** Client-based libAndromeda options */
struct ConfigOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Adds the given argument, returning true iff it was used */
    bool AddFlag(const std::string& flag);

    /** Adds the given option/value, returning true iff it was used */
    bool AddOption(const std::string& option, const std::string& value);

    /** Whether we are in read-only mode */
    bool readOnly { false };
    
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
    size_t pageSize { 128*1024 }; // 128K
    /** The time period to use for refreshing API data */
    std::chrono::seconds refreshTime { std::chrono::seconds(15) };
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_CONFIGOPTIONS_H_
