
#ifndef LIBA2_UTILITIES_H_
#define LIBA2_UTILITIES_H_

#include <vector>
#include <string>
#include <utility>
#include <mutex>
#include <sstream>
#include <stdexcept>

class AndromedaException : public std::runtime_error { 
    using std::runtime_error::runtime_error; };

class Utilities
{
public:
    template <class T>
    static std::string implode(T arr, const std::string& delim)
    {
        std::string retval;
        for (const std::string& piece : arr) 
            retval += piece;
        return std::move(retval);
    }

    static std::vector<std::string> explode(
        std::string str, const std::string& delim, 
        const int max = -1, const size_t skip = 0);
};

class Debug
{
public:

    enum class Level
    {
        DEBUG_NONE,
        DEBUG_BASIC,
        DEBUG_DETAILS
    };

    Debug(const std::string& prefix, void* addr = nullptr) : 
        addr(addr), prefix(prefix) { }

    static void SetLevel(Level level){ Debug::level = level; }

    void Out(Level level, const std::string& str = "");

    void Print(const std::string& str = "");

    Debug& operator<<(const std::string& str);

private:

    void* addr;
    std::string prefix;
    std::ostringstream buffer;

    static Level level;
    static std::mutex mutex;
};

#endif