#ifndef LIBA2_CONFIGOPTIONS_H_
#define LIBA2_CONFIGOPTIONS_H_

#include <chrono>
#include <cstdint>
#include <string>

namespace Andromeda {

/** Client-based libAndromeda options */
struct ConfigOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Adds the given argument, returning true iff it was used */
    bool AddFlag(const std::string& flag);

    /** 
     * Adds the given option/value, returning true iff it was used
     * @throws BaseOptions::Exception if invalid arguments
     */
    bool AddOption(const std::string& option, const std::string& value);

    /** True if terminal prompting is not allowed */
    bool quiet { false };

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
     * and memory efficiency, but might increase latency for small transfers made bigger by the pageSize.
     * This should be >= and a multiple of the OS's page size (4K Linux, 64K Windows) or memory will be wasted.
     * Maybe ~1M can be good for fast sequential cases with a large cache. 4K can help with IOPS on small random
     * writes especially if the file is larger than the total cache. 128K is a good compromise for most use cases.  
     */
    size_t pageSize { 131072 }; // 128K

    /** 
     * The target transfer time for each read-ahead page fetch 
     * Uses bandwidth measuring to convert this time target to an actual page count
     */
    std::chrono::milliseconds readAheadTime { 2000 };

    /** 
     * The maximum fraction of the cache that a read-ahead can consume (1/x)
     * E.g. if the cache max is 256MB and frac is 4, no read can be > 64MB regardless of time
     * Smaller values could increase total throughput but will lower cache effectiveness
     */
    uint32_t readMaxCacheFrac { 4 };

    /** 
     * The number of pages past the current to always pre-populate
     * E.g. if readAhead is 2, then reading page 0 starts a fetch if 0,1,2 are not all populated
     * Larger values may lower latency and increase total throughput at the cost of higher
     * CPU usage and possibly wasted bandwidth and cache. Overall effect is small either way.
     */
    size_t readAheadBuffer { 2 };

    /** The maximum number of concurrent backend runners, never zero! */
    size_t runnerPoolSize { 1 }; // TODO server has threading issues
};

} // namespace Andromeda

#endif // LIBA2_CONFIGOPTIONS_H_
