#ifndef LIBA2_UTILITIES_H_
#define LIBA2_UTILITIES_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Clang 10 gives Wsizeof-array-div without extra() around sizeof
#define ARRSIZE(x) (sizeof(x)/(sizeof(decltype(*x))))

namespace Andromeda {

/** Misc Utilities */
class Utilities
{
public:

    Utilities() = delete; // static only

    typedef std::vector<std::string> StringList;

    /**
     * Split a string into an array
     * @param str string to split
     * @param delim string separating pieces
     * @param skip the number of delims to skip
     * @param reverse if true, find delims from end
     * @param max max # of elements to return
     */
    static StringList explode(
        std::string str, const std::string& delim, 
        const size_t skip = 0, const bool reverse = false,
        const size_t max = static_cast<size_t>(-1));

    typedef std::pair<std::string,std::string> StringPair;

    /** 
     * Special case of explode with max=1,skip=0 and returns a pair 
     * @param str string to split
     * @param delim string separating pieces
     * @param skip the number of delims to skip
     * @param reverse if true, find delims from end
     */
    static StringPair split(
        const std::string& str, const std::string& delim, 
        const size_t skip = 0, const bool reverse = false);

    /** Splits a path into its dirname and basename */
    static StringPair splitPath(const std::string& str); // TODO unit test

    /** Returns true iff str starts with start */
    static bool startsWith(const std::string& str, const std::string& start);

    /** Returns true iff str ends with end */
    static bool endsWith(const std::string& str, const std::string& end);

    /** Returns the string with leading/trailing whitespace stripped */
    static std::string trim(const std::string& str);

    /** Returns the string str with all occurences of from replaced by to */
    static std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);

    /** Returns false if the trimmed string is a false-like value */
    static bool stringToBool(const std::string& stri);

    /** 
     * Converts a string like "4096" or "256M" to # of bytes
     * @throws std::logic_error if we fail to convert to a number
     */
    static uint64_t stringToBytes(const std::string& stri);

    /**
     * Converts # of bytes to a string like "256K" or "4M"
     * stopping at the biggest possible unit (whole numbers only)
     */
    static std::string bytesToString(uint64_t bytes);

    /**
     * Silently read a line of input from stdin
     * @param[out] retval reference to string to fill
     */
    static void SilentReadConsole(std::string& retval);

    typedef std::unordered_map<std::string,std::string> StringMap;

    /** 
     * Returns a string map of the process environment variables 
     * @param prefix if not empty, only return env vars whose keys start with this
     */
    static StringMap GetEnvironment(const std::string& prefix = "");

    /** Returns the user's home directory path if found */
    static std::string GetHomeDirectory();

    /** Returns strerror(err) but thread safe */
    static std::string GetErrorString(int err);
};

} // namespace Andromeda

#endif // LIBA2_UTILITIES_H_
