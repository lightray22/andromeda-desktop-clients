#ifndef LIBA2_DEBUG_H_
#define LIBA2_DEBUG_H_

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

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

    /** Returns the highest level of all configured streams */
    static Level GetLevel(){ return sMaxLevel; }

    /** Sets the configured log level for all streams - THREAD SAFE */
    static void SetLevel(Level level);

    /** Sets the configured log level for the given stream - THREAD SAFE */
    static void SetLevel(Level level, const std::ostream& stream);

    /** Sets the list of comma-separated filters for all streams - THREAD SAFE */
    static void SetFilters(const std::string& filters);

    /** Sets the list of comma-separated filters for the given stream - THREAD SAFE */
    static void SetFilters(const std::string& filters, const std::ostream& stream);

    /** Adds an output stream reference to send output to - THREAD SAFE */
    static void AddStream(std::ostream& stream);

    /** Removes an output stream reference added earlier - THREAD SAFE */
    static void RemoveStream(std::ostream& stream);

    /** 
     * Adds a file output stream to send output to - THREAD SAFE 
     * copies level/filters from the first existing stream
    */
    static void AddLogFile(const std::string& path);

    /**
     * Construct a new debug module (simple and static safe!)
     * @param prefix name to use for all prints
     * @param addr address to print with details
     */
    explicit Debug(const std::string& prefix, void* addr) noexcept : 
        mAddr(addr), mPrefix(prefix) { }

    /** Function to send debug text to a given output stream */
    using StreamFunc = std::function<void (std::ostream&)>;

    /** Prints func to cerr if the level is >= ERRORS */
    inline void Error(const StreamFunc& strfunc)
    {
        if (sMaxLevel >= Level::ERRORS) Print(strfunc,Level::ERRORS);
    }

    /** Prints func to cerr if the level is >= BACKEND */
    inline void Backend(const StreamFunc& strfunc)
    {
        if (sMaxLevel >= Level::BACKEND) Print(strfunc,Level::BACKEND);
    }

    /** Prints func to cerr if the level is >= INFO */
    inline void Info(const StreamFunc& strfunc)
    {
        // caching sMaxLevel lets us do this very quick check here
        if (sMaxLevel >= Level::INFO) Print(strfunc,Level::INFO);
    }

    /** Syntactic sugar to send the current function name and strcode to debug (error) */
    #define DBG_ERROR(debug, strcode) { const char* const myfname { __func__ }; \
        debug.Error([&](std::ostream& str){ str << myfname << strcode; }); }

    /** Syntactic sugar to send the current function name and strcode to debug (info) */
    #define DBG_INFO(debug, strcode) { const char* const myfname { __func__ }; \
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
    static StreamFunc DumpBytes(const void* ptr, size_t bytes, size_t width = 16);

private:

    /** 
     * Prints func to all registered streams with other info - THREAD SAFE
     * @param level level of the calling function, will not use filters if ERRORS
     */
    void Print(const StreamFunc& strfunc, Level level);

    /** The address this debug instance belongs to */
    void* const mAddr { nullptr };
    /** The module name this debug instance belongs to */
    std::string mPrefix;

    static std::mutex sMutex;
    /** timestamp when the program started */
    static std::chrono::steady_clock::time_point sStart;

    /** A sink for debug output */
    struct Context
    {
        std::ostream* stream;
        Level level { Debug::Level::ERRORS };
        std::unordered_set<std::string> filters;

        explicit Context(std::ostream& s) : stream(&s){ }
    };

    /** Converts a comma-separated string of filters to a filter set */
    static decltype(Context::filters) GetFilterSet(const std::string& filters);

    /** Get the max level of all contexts */
    static Level GetMaxLevel();

    /** Global set of debug output sinks - using a vector instead of map for maximum 
     * iteration performance - adding/modifying streams is not performance-sensitive! */
    static std::vector<Context> sContexts;

    /** The maximum level of all contexts, for minimum overhead
     * for debug calls in the usual case when debug is off */
    static Level sMaxLevel;

    /** Subset list of file streams that we own */
    static std::list<std::ofstream> sFileStreams;
};

} // namespace Andromeda

#endif // LIBA2_DEBUG_H_
