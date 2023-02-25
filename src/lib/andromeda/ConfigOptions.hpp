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
     * The time period to use for refreshing folder data 
     * Smaller values refresh more quickly but access the backend more
     */
    std::chrono::seconds refreshTime { 15 };

    /** 
     * The default file data page size 
     * The minimum of a file's size and its pageSize is the smallest unit of data that can be read from or 
     * written to the backend.  Higher page sizes may increase maximum sequential bandwidth and increase CPU 
     * and memory efficiency, but will increase latency for small transfers made bigger by the pageSize.
     * 128K is a good default for most use cases. Do not make smaller than 16K on Linux glibc (or ever?).
     */
    size_t pageSize { 128*1024 }; // 128K

    /** 
     * The target transfer time for each read-ahead page fetch 
     * Uses bandwidth measuring to convert this time target to an actual page count
     */
    std::chrono::milliseconds readAheadTime { 1000 };

    /** 
     * The maximum fraction of the cache that a read-ahead can consume (1/x) 
     * E.g. if the cache max is 256MB and frac is 4, no read can be > 64MB regardless of time
     * Smaller values could increase total throughput but will lower cache effectiveness
     */
    size_t readMaxCacheFrac { 4 };

    /** 
     * The number of pages past the current to always pre-populate 
     * E.g. if readAhead is 2, then reading page 0 starts a fetch if 0,1,2 are not all populated
     * Larger values may lower latency and increase total throughput at the cost of higher
     * CPU usage and possibly wasted bandwidth and cache. Overall effect is small.
     */
    size_t readAheadBuffer { 2 };
};

} // namespace Andromeda

#endif // LIBA2_CONFIGOPTIONS_H_
