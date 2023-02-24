#ifndef LIBA2_CONFIGOPTIONS_H_
#define LIBA2_CONFIGOPTIONS_H_

#include <chrono>
#include <string>

namespace Andromeda {

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

    /** 
     * The default file data page size 
     * The minimum of a file's size and its pageSize is the smallest unit of data that can be read from or 
     * written to the backend.  Higher page sizes may increase maximum sequential bandwidth and increase CPU 
     * and memory efficiency, but will increase latency for small transfers made bigger by the pageSize.
     * 128K is a good default for most use cases. Do not make smaller than 16K on Linux glibc (or ever?).
     */
    size_t pageSize { 128*1024 }; // 128K

    /** The time period to use for refreshing API data */
    std::chrono::seconds refreshTime { std::chrono::seconds(15) };
};

} // namespace Andromeda

#endif // LIBA2_CONFIGOPTIONS_H_
