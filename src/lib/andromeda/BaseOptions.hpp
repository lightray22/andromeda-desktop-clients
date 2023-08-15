#ifndef LIBA2_BASEOPTIONS_H_
#define LIBA2_BASEOPTIONS_H_

#include <filesystem>
#include <list>
#include <map>
#include <stdexcept>
#include <string>

#include "BaseException.hpp"
#include "Debug.hpp"

namespace Andromeda {

/** Common user options */
class BaseOptions
{
public:

    virtual ~BaseOptions() = default;

    /** Base class for all Options errors */
    class Exception : public BaseException {
        using BaseException::BaseException; };

    /** Exception indicating help text should be shown */
    class ShowHelpException : public Exception {
        public: ShowHelpException() : Exception("") {} };

    /** Exception indicating the version should be shown */
    class ShowVersionException : public Exception {
        public: ShowVersionException() : Exception("") {} };

    /** Exception indicating the command usage was bad */
    class BadUsageException : public Exception { 
        public: explicit BadUsageException(const std::string& details) : 
            Exception("Invalid Usage: "+details) {} };

    /** Exception indicating the given flag is unknown */
    class BadFlagException : public Exception { 
        /** @param flag the unknown flag */
        public: explicit BadFlagException(const std::string& flag) : 
            Exception("Unknown Flag: "+flag) {} };

    /** Exception indicating the given option is unknown */
    class BadOptionException : public Exception { 
        /** @param option the unknown option */
        public: explicit BadOptionException(const std::string& option) : 
            Exception("Unknown Option: "+option) {} };

    /** Exception indicating the given option has a bad value */
    class BadValueException : public Exception {
        /** @param option the option name */
        public: explicit BadValueException(const std::string& option) :
            Exception("Bad Option Value: "+option) {} };

    /** Exception indicating the given option is required but not present */
    class MissingOptionException : public Exception {
        /** @param option the option name */
        public: explicit MissingOptionException(const std::string& option) :
            Exception("Missing Option: "+option) {} };

    using Flags = std::list<std::string>;
    using Options = std::multimap<std::string, std::string>;

    /** 
     * Parses command line arguments from main (skips argv[0]!) 
     * @throws Exception if invalid arguments
     * @param stopmm if true, stop processing when "--" is encountered
     * @return number of arguments consumed (matches argc if stopmm is false)
     */
    size_t ParseArgs(size_t argc, const char* const* argv, bool stopmm = false);

    /** 
     * Parses arguments from a config file 
     * @throws Exception if invalid arguments
     */
    void ParseFile(const std::filesystem::path& path);

    /**
     * Finds and parses arguments from a config file
     * @param prefix the name of the config file to find
     * @throws Exception if invalid arguments
     */
    void ParseConfig(const std::string& prefix);

    /** 
     * Parses optional arguments from URL variables
     * @throws Exception if invalid arguments
     */
    void ParseUrl(const std::string& url);

    /** 
     * Adds the given argument, returning true iff it was used
     * @throws Exception if invalid arguments
     * @throws ShowHelpException if help text is requested
     * @throws ShowVersionException if version text is requested
     */
    virtual bool AddFlag(const std::string& flag);

    /** 
     * Adds the given option/value, returning true iff it was used 
     * @throws Exception if invalid arguments
     */
    virtual bool AddOption(const std::string& option, const std::string& value);

    /** 
     * Adds the given URL argument if applicable
     * @throws Exception if invalid arguments
     */
    virtual void TryAddUrlFlag(const std::string& flag) { };

    /** 
     * Adds the given URL option/value if applicable
     * @throws Exception if invalid arguments
     */
    virtual void TryAddUrlOption(const std::string& option, const std::string& value) { };

    /** 
     * Makes sure all required options were provided 
     * @throws MissingOptionException if a required option is missing
     */
    virtual void Validate() = 0;

protected:

    /** Retrieve the standard help text string */
    static std::string CoreBaseHelpText();

    /** 
     * Retrieve the standard help text string 
     * @param name suffix of andromeda-*.conf the user can use (or blank)
     */
    static std::string DetailBaseHelpText(const std::string& name = "");
};

} // namespace Andromeda

#endif // LIBA2_BASEOPTIONS_H_
