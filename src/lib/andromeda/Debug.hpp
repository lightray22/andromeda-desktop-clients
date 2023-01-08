#ifndef LIBA2_DEBUG_H_
#define LIBA2_DEBUG_H_

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
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

    /** Returns the configured global debug level */
    static Level GetLevel() { return sLevel; }

    /** Sets the configured global debug level */
    static void SetLevel(Level level){ sLevel = level; }

    /**
     * @param prefix to use for all prints
     * @param addr address to print with details
     */
    explicit Debug(const std::string& prefix, void* addr) : 
        mAddr(addr), mPrefix(prefix) { }

    /** Function to send debug text to a given output stream */
    typedef std::function<void(std::ostream& str)> StreamFunc;

    /** Prints func to cerr if the level is >= ERRORS */
    inline void Error(StreamFunc strfunc)
    {
        if (sLevel >= Level::ERRORS) Print(strfunc);
    }

    /** Prints func to cerr if the level is >= BACKEND */
    inline void Backend(StreamFunc strfunc)
    {
        if (sLevel >= Level::BACKEND) Print(strfunc);
    }

    /** Prints func to cerr if the level is >= INFO */
    inline void Info(StreamFunc strfunc)
    {
        if (sLevel >= Level::INFO) Print(strfunc);
    }

    /** Syntactic sugar to send the current function name and strcode to debug (error) */
    #define DBG_ERROR(debug, strcode) { const char* myfname { __func__ }; \
        debug.Error([&](std::ostream& str){ str << myfname << strcode; }); }

    /** Syntactic sugar to send the current function name and strcode to debug (info) */
    #define DBG_INFO(debug, strcode) { const char* myfname { __func__ }; \
        debug.Info([&](std::ostream& str){ str << myfname << strcode; }); }

    #define DDBG_ERROR(strfunc) DBG_ERROR(debug, strfunc)
    #define MDBG_ERROR(strfunc) DBG_ERROR(mDebug, strfunc)
    #define SDBG_ERROR(strfunc) DBG_ERROR(sDebug, strfunc)
    
    #define DDBG_INFO(strfunc) DBG_INFO(debug, strfunc)
    #define MDBG_INFO(strfunc) DBG_INFO(mDebug, strfunc)
    #define SDBG_INFO(strfunc) DBG_INFO(sDebug, strfunc)

    /**
     * Returns a StreamFunc to print a struct as hex to the buffer
     * @param ptr address of struct to print
     * @param bytes number of bytes to print
     * @param width number of bytes per line
     */
    static StreamFunc DumpBytes(const void* ptr, uint64_t bytes, uint8_t width = 16);

private:

    /** Prints func to cerr with other info - THREAD SAFE */
    void Print(StreamFunc& getDebug);

    void* mAddr;
    std::string mPrefix;

    static Level sLevel;
    static std::mutex sMutex;

    static std::chrono::high_resolution_clock::time_point sStart;
};

} // namespace Andromeda

#endif // LIBA2_DEBUG_H_
