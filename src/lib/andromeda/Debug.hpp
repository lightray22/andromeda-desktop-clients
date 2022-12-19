#ifndef LIBA2_DEBUG_H_
#define LIBA2_DEBUG_H_

#include <chrono>
#include <mutex>
#include <sstream>
#include <string>

namespace Andromeda {

/** Global thread-safe debug printing */
class Debug
{
public:

    /** Debug verbosity */
    enum class Level
    {
        /** Debug off */          NONE,    
        /** Only show Error()s */ ERRORS,  
        /** Also show Backend */  BACKEND, 
        /** Everything else */    INFO,    
        /** Show extra details */ DETAILS  
    };

    /**
     * @param prefix to use for all prints
     * @param addr address to print with details
     */
    explicit Debug(const std::string& prefix, void* addr) : 
        mAddr(addr), mPrefix(prefix) { }

    /** Returns the configured global debug level */
    static Level GetLevel() { return sLevel; }

    /** Sets the configured global debug level */
    static void SetLevel(Level level){ sLevel = level; }

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

    /**
     * Adds a struct as hex bytes to the buffer
     * @param ptr address of struct to print
     * @param bytes number of bytes to print
     * @param width number of bytes per line
     */
    void DumpBytes(const void* ptr, uint64_t bytes, uint8_t width = 16);

    /** Append to an internal buffer that can be shown with an empty Print */
    template <class T> Debug& operator<<(const T& dat)
    {
        const std::lock_guard<decltype(sMutex)> lock(sMutex);
        
        if (static_cast<bool>(sLevel))
            mBuffer << dat;
        return *this;
    }

    /** Returns true iff debug is enabled */
    operator bool() const;

private:

    void* mAddr;
    std::string mPrefix;
    std::ostringstream mBuffer;

    static Level sLevel;
    static std::mutex sMutex;

    static std::chrono::high_resolution_clock::time_point sStart;
};

} // namespace Andromeda

#endif // LIBA2_DEBUG_H_
