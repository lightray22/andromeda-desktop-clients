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

    virtual ~BaseOptions(){ };

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
        public: BadUsageException() : 
            Exception("Invalid Usage") {} };

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

    typedef std::list<std::string> Flags;
    typedef std::multimap<std::string, std::string> Options;

    /** 
     * Parses command line arguments from main (skips argv[0]!) 
     * @throws BadUsageException if invalid arguments
     * @throws BadFlagException if a invalid flag is used
     * @throws BadOptionException if an invalid option is used
     */
    void ParseArgs(int argc, const char* const* argv);

    /** 
     * Parses arguments from a config file 
     * @throws BadUsageException if invalid arguments
     * @throws BadFlagException if a invalid flag is used
     */
    void ParseFile(const std::filesystem::path& path);

    /** Parses optional arguments from URL variables */
    void ParseUrl(const std::string& url);

    /** Adds the given argument, returning true iff it was used */
    virtual bool AddFlag(const std::string& flag);

    /** 
     * Adds the given option/value, returning true iff it was used 
     * @throws BadValueException if a bad option value is used
     */
    virtual bool AddOption(const std::string& option, const std::string& value);

    /** Adds the given URL argument if applicable */
    virtual void TryAddUrlFlag(const std::string& flag) { };

    /** Adds the given URL option/value if applicable */
    virtual void TryAddUrlOption(const std::string& option, const std::string& value) { };

    /** 
     * Makes sure all required options were provided 
     * @throws MissingOptionException if a required option is missing
     */
    virtual void Validate() { };

    /** Returns the desired debug level */
    Debug::Level GetDebugLevel() const { return mDebugLevel; }

protected:

    /** Retrieve the standard help text string */
    static std::string CoreBaseHelpText();

    /** Retrieve the standard help text string */
    static std::string OtherBaseHelpText();

private:

    Debug::Level mDebugLevel { Debug::Level::NONE };
};

} // namespace Andromeda

#endif // LIBA2_BASEOPTIONS_H_
