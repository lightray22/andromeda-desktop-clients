#ifndef A2CLI_COMMANDLINE_H_
#define A2CLI_COMMANDLINE_H_

#include <fstream>
#include <list>
#include <string>

#include "andromeda/BaseOptions.hpp"

class Options;

namespace Andromeda { namespace Backend { 
    class HTTPRunner; 
    struct RunnerInput; 
    struct RunnerInput_StreamIn;
    struct RunnerInput_StreamOut;
} }

/** Gets options and a remote Input from the command line */
class CommandLine
{
public:

    /** Exception indicating that stream out and files input are incompatible */
    class IncompatibleIOException : public Andromeda::BaseOptions::Exception { 
        public: IncompatibleIOException() : Andromeda::BaseOptions::Exception("Cannot stream output with file input") {} };

    virtual ~CommandLine();

    /** Retrieve the standard help text string */
    static std::string HelpText();

    explicit CommandLine(Options& options);

    /** 
     * Parses command line arguments from main (skips argv[0]!)
     * and the environment into Options and a RunnerInput 
     * @throws BadUsageException if invalid arguments
     * @throws BadFlagException if a invalid flag is used
     * @throws BadOptionException if an invalid option is used
     */
    void ParseFullArgs(size_t argc, const char* const* argv);

    /** 
     * Returns the runner input from the command line (AFTER ParseFullArgs!)
     * @param runner reference to the HTTP runner to use
     * @param isJson set to whether the response is JSON or not
     */
    std::string RunInputAction(Andromeda::Backend::HTTPRunner& runner, bool& isJson);

private:

    Options& mOptions;
    std::list<std::ifstream> mOpenFiles;

    std::unique_ptr<Andromeda::Backend::RunnerInput> mInput;
    std::unique_ptr<Andromeda::Backend::RunnerInput_StreamIn> mInput_StreamIn;
    std::unique_ptr<Andromeda::Backend::RunnerInput_StreamOut> mInput_StreamOut;
};

#endif // A2CLI_COMMANDLINE_H_
