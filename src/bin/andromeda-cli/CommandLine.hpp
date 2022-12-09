#ifndef A2CLI_COMMANDLINE_H_
#define A2CLI_COMMANDLINE_H_

#include <string>

#include "andromeda/backend/RunnerInput.hpp"

class Options;

/** Gets options and a remote Input from the command line */
class CommandLine
{
public:
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

    /** Returns the runner input from the command line */
    const Andromeda::Backend::RunnerInput& GetRunnerInput() { return mInput; }

private:

    Options& mOptions;
    Andromeda::Backend::RunnerInput mInput;
};

#endif // A2CLI_COMMANDLINE_H_
