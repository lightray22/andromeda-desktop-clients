#ifndef A2CLI_COMMANDLINE_H_
#define A2CLI_COMMANDLINE_H_

#include <fstream>
#include <list>
#include <string>

#include "andromeda/BaseOptions.hpp"
#include "andromeda/Utilities.hpp"
#include "andromeda/backend/RunnerInput.hpp"

namespace Andromeda { namespace Backend { class HTTPRunner; } }

namespace AndromedaCli {

class Options;

/** Gets options and a remote Input from the command line */
class CommandLine
{
public:

    /** Exception indicating that stream out and files input are incompatible */
    class IncompatibleIOException : public Andromeda::BaseOptions::Exception { 
        public: IncompatibleIOException() : Andromeda::BaseOptions::Exception("Cannot stream output with file input") {} };

    /** Exception indicating the given param key is probably not safe for a URL */
    class PrivateDataException : public Andromeda::BaseOptions::Exception {
        public: explicit PrivateDataException(const std::string& key) : 
            Andromeda::BaseOptions::Exception(key + " is not safe to send as a URL variable, use env or stdin instead") {} };

    /** Retrieve the standard help text string */
    static std::string HelpText();

    /** 
     * Parses command line arguments from main (skips argv[0]!)
     * and the environment vars into Options and a RunnerInput 
     * @throws BadUsageException if invalid arguments
     * @throws BadFlagException if a invalid flag is used
     * @throws BadOptionException if an invalid option is used
     */
    explicit CommandLine(Options& options, size_t argc, const char* const* argv);

    /** 
     * Returns the runner input from the command line
     * @param[in] runner reference to the HTTP runner to use
     * @param[out] isJson set to whether the return value is JSON or not
     * @param[in] streamOut function to use for output streaming
     */
    std::string RunInputAction(Andromeda::Backend::HTTPRunner& runner, bool& isJson, 
        const Andromeda::Backend::ReadFunc& streamOut);

private:

    /**
     * Returns the next non-key value after i if it exists
     * @param argv command line argument list
     * @param i current index to check after
     */
    std::string getNextValue(const Andromeda::Utilities::StringList& argv, size_t& i);

    /**
     * Processes Andromeda action params from the command line
     * This code is basically the same as the server's CLI.php
     * @param args input list of command line arguments
     * @param isPriv true if these params are private (dataParams only)
     * @param[out] plainParams plain parameters ref to fill
     * @param[out] dataParams data parameters ref to fill
     * @param[out] inStreams file input streams ref to fill
     */
    void ProcessArgList(const Andromeda::Utilities::StringList& args, bool isPriv,
        Andromeda::Backend::RunnerInput::Params& plainParams, 
        Andromeda::Backend::RunnerInput::Params& dataParams,
        Andromeda::Backend::RunnerInput_StreamIn::FileStreams& inStreams);

    Options& mOptions;
    std::list<std::ifstream> mOpenFiles;

    std::unique_ptr<Andromeda::Backend::RunnerInput> mInput;
    std::unique_ptr<Andromeda::Backend::RunnerInput_StreamIn> mInput_StreamIn;
    std::unique_ptr<Andromeda::Backend::RunnerInput_StreamOut> mInput_StreamOut;
};

} // namespace AndromedaCli

#endif // A2CLI_COMMANDLINE_H_
