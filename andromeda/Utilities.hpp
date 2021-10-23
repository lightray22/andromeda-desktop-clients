#ifndef LIBA2_UTILITIES_H_
#define LIBA2_UTILITIES_H_

#include <map>
#include <list>
#include <vector>
#include <string>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <utility>

#define A2LIBVERSION "0.1-alpha"

/** Misc Utilities */
class Utilities
{
public:

    /** Base Exception for all Andromeda errors */
    class Exception : public std::runtime_error { 
        using std::runtime_error::runtime_error; };

    /**
     * Collapse an array into a string
     * @param arr container with strings
     * @param delim character to put between entries
     * @return imploded string
     */ 
    template <class T>
    static std::string implode(T arr, const std::string& delim)
    {
        std::string retval;
        for (const std::string& piece : arr) 
            retval += piece;
        return retval;
    }

    typedef std::vector<std::string> StringList;

    /**
     * Split a string into an array
     * @param str string to split
     * @param delim string separating pieces
     * @param max max # of elements to return
     * @param skip the number of delims to skip
     */
    static StringList explode(
        std::string str, const std::string& delim, 
        const int max = -1, const size_t skip = 0);

    typedef std::pair<std::string,std::string> StringPair;

    /** 
     * Special case of explode with max=1,skip=0 and returns a pair 
     * @param last if true, find last delim else first
     */
    static StringPair split(const std::string& str, 
        const std::string& delim, const bool last = false);

    typedef std::list<std::string> Flags;
    typedef std::map<std::string, std::string> Options;

    /** 
     * Parses argc/argv C arguments into a flag list and option map 
     * @return bool false if invalid arguments were given
     */
    static bool parseArgs(int argc, char** argv, Flags& flags, Options& options);

    /** Returns false if the given string is a false-like value */
    static bool stringToBool(const std::string& str);

    /**
     * Silently read a line of input from stdin
     * @param retval reference to string to fill
     */
    static void SilentReadConsole(std::string& retval);    
};

/** Global thread-safe debug printing */
class Debug
{
public:

    /** Debug verbosity */
    enum class Level
    {
        NONE,    /** Debug off */
        ERRORS,  /** Only show Error()s */
        BACKEND, /** Also show Backend */
        INFO,    /** Everything else */
        DETAILS  /** Show extra details */
    };

    /**
     * @param prefix to use for all prints
     * @param addr address to print with details
     */
    Debug(const std::string& prefix, void* addr = nullptr) : 
        addr(addr), prefix(prefix) { }

    /** Returns the configured global debug level */
    static Level GetLevel() { return level; }

    /** Sets the configured global debug level */
    static void SetLevel(Level level){ Debug::level = level; }

    /**
     * Shows the given debug string with minlevel=INFO
     * @param str the string to show, or "" to use the buffer
     */
    void Info(const std::string& str = "");

    /**
     * Shows the given debug string with minlevel=BACKEND
     * @param str the string to show, or "" to use the buffer
     */
    void Backend(const std::string& str = "");

    /**
     * Shows the given debug string with minlevel=ERRORS
     * @param str string to show, or "" to use the buffer
     */
    void Error(const std::string& str = "");

    /** Append to an internal buffer that can be shown with an empty Print */
    template <class T> Debug& operator<<(const T& dat)
    {
        if (static_cast<bool>(level))
            this->buffer << dat;
        return *this;
    }

    /** Returns true iff debug is enabled */
    operator bool() const;

private:

    void* addr;
    std::string prefix;
    std::ostringstream buffer;

    static Level level;
    static std::mutex mutex;

    static std::chrono::high_resolution_clock::time_point start;
};

#endif