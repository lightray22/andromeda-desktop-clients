#ifndef LIBA2_RUNNEROPTIONS_H_
#define LIBA2_RUNNEROPTIONS_H_

#include <chrono>
#include <cstdint>
#include <string>

namespace Andromeda {
namespace Backend {

/** Runner config options */
struct RunnerOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** Adds the given argument, returning true iff it was used */
    bool AddFlag(const std::string& flag){ return false; }

    /** 
     * Adds the given option/value, returning true iff it was used
     * @throws BaseOptions::Exception if invalid arguments
     */
    bool AddOption(const std::string& option, const std::string& value);

    using seconds = std::chrono::seconds;

    /** maximum retries before throwing */
    uint32_t maxRetries { 4 };
    /** The time to wait between each retry */
    seconds retryTime { 3 };
    /** The connection read/write timeout */
    seconds timeout { 60 };
    /** Buffer/chunk size when reading file streams */
    size_t streamBufferSize { 1048576 }; // 1M
};

} // namespace Backend
} // namespace Andromeda

#endif // LIBA2_RUNNEROPTIONS_H_
