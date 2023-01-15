#ifndef LIBA2_DEBUG_H_
#define LIBA2_DEBUG_H_

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_set>

namespace Andromeda {

/** Global thread-safe debug printing */
class Debug
{
public:

    /** Debug verbosity */
    enum class Level
    {
        /** Only show Error()s */ ERRORS,  
        /** Also show Backend */  BACKEND, 
        /** Everything else */    INFO,    
        /** Show extra details */ DETAILS,
        LAST = DETAILS
    };

    /** Returns the configured global debug level */
    static Level GetLevel(){ return sLevel; }

    /** Sets the configured global debug level */
    static void SetLevel(Level level){ sLevel = level; }

    /** Adds the given component name to the filter set */
    static void AddFilter(const std::string& name){ sPrefixes.emplace(name); }

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
        if (sLevel >= Level::BACKEND) PrintIf(strfunc);
    }

    /** Prints func to cerr if the level is >= INFO */
    inline void Info(StreamFunc strfunc)
    {
        if (sLevel >= Level::INFO) PrintIf(strfunc);
    }

    /** Syntactic sugar to send the current function name and strcode to debug (error) */
    #define DBG_ERROR(debug, strcode) { static const std::string myfname(__func__); \
        debug.Error([&](std::ostream& str){ str << myfname << strcode; }); }

    /** Syntactic sugar to send the current function name and strcode to debug (info) */
    #define DBG_INFO(debug, strcode) { static const std::string myfname(__func__); \
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

    /** Prints func to cerr with other info IF the prefix is enabled - THREAD SAFE */
    void PrintIf(StreamFunc& strfunc);

    /** Prints func to cerr with other info - THREAD SAFE */
    void Print(StreamFunc& strfunc);

    /** The address this debug instance belongs to */
    void* mAddr;
    /** The module name this debug instance belongs to */
    std::string mPrefix;

    /** Global debug level */
    static Level sLevel;
    /** Global output lock */
    static std::mutex sMutex;

    /** Set of prefixes to filter printing */
    static std::unordered_set<std::string> sPrefixes;

    /** timestamp when the program started */
    static std::chrono::high_resolution_clock::time_point sStart;
};

} // namespace Andromeda

#endif // LIBA2_DEBUG_H_
