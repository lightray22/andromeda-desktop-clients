#ifndef LIBA2_PLATFORMUTIL_H_
#define LIBA2_PLATFORMUTIL_H_

#include <string>
#include <unordered_map>

namespace Andromeda {

/** Platform abstractions */
class PlatformUtil
{
public:

    PlatformUtil() = delete; // static only

    /**
     * Silently read a line of input from stdin
     * @param[out] retval reference to string to fill
     */
    static void SilentReadConsole(std::string& retval);

    using StringMap = std::unordered_map<std::string, std::string>;

    /** 
     * Returns a string map of the process environment variables 
     * @param prefix if not empty, only return env vars whose keys start with this
     */
    [[nodiscard]] static StringMap GetEnvironment(const std::string& prefix = "");

    /** Returns the user's home directory path if found */
    [[nodiscard]] static std::string GetHomeDirectory();

    /** Returns strerror(err) but thread safe */
    [[nodiscard]] static std::string GetErrorString(int err);
};

} // namespace Andromeda

#endif // LIBA2_PLATFORMUTIL_H_
