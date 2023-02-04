#ifndef LIBA2_UTILITIES_H_
#define LIBA2_UTILITIES_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

    /** Returns true iff str starts with start */
    static bool startsWith(const std::string& str, const std::string& start);

    /** Returns true iff str ends with end */
    static bool endsWith(const std::string& str, const std::string& end);

    /** Returns the string with leading/trailing whitespace stripped */
    static std::string trim(const std::string& str);

    /** Returns false if the trimmed string is a false-like value */
    static bool stringToBool(const std::string& stri);

    /**
     * Silently read a line of input from stdin
     * @param retval reference to string to fill
     */
    static void SilentReadConsole(std::string& retval);

    typedef std::unordered_map<std::string,std::string> StringMap;

    /** Returns a string map of the process environment variables */
    static StringMap GetEnvironment();

    /** Returns the user's home directory path if found */
    static std::string GetHomeDirectory();

    /** Returns strerror(err) but thread safe */
    static std::string GetErrorString(int err);
};

} // namespace Andromeda

#endif // LIBA2_UTILITIES_H_
