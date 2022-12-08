#ifndef LIBA2_UTILITIES_H_
#define LIBA2_UTILITIES_H_

#include <string>
#include <vector>

namespace Andromeda {

/** Misc Utilities */
class Utilities
{
public:

    /** A vector of strings */
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

    /** A pair of two strings */
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

    /** Returns false if the trimmed string is a false-like value */
    static bool stringToBool(std::string str);

    /**
     * Silently read a line of input from stdin
     * @param retval reference to string to fill
     */
    static void SilentReadConsole(std::string& retval);    
};

} // namespace Andromeda

#endif

