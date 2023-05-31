
#include <sstream>

#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;

namespace AndromedaCli {

/*****************************************************/
std::string Options::CoreHelpText()
{
    return CoreBaseHelpText();
}

/*****************************************************/
std::string Options::MainHelpText()
{
    return "-a|--apiurl url";
}

/*****************************************************/
std::string Options::DetailHelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Other Options:   [--stream-out] [--allow-unsafe-url]" << endl
        << HTTPOptions::HelpText() << endl 
        << RunnerOptions::HelpText() << endl << endl
           
        << DetailBaseHelpText("cli");

    return output.str();
}

/*****************************************************/
Options::Options(HTTPOptions& httpOptions, RunnerOptions& runnerOptions) :
    mHttpOptions(httpOptions), mRunnerOptions(runnerOptions) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (BaseOptions::AddFlag(flag)) { }

    else if (flag == "stream-out") mStreamOut = true;
    else if (flag == "allow-unsafe-url") mUnsafeUrl = true;

    else if (mHttpOptions.AddFlag(flag)) { }
    else if (mRunnerOptions.AddFlag(flag)) { }
    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    if (BaseOptions::AddOption(option, value)) { }

    /** Backend endpoint selection */
    else if (option == "a" || option == "apiurl") mApiUrl = value;

    else if (mHttpOptions.AddOption(option, value)) { }
    else if (mRunnerOptions.AddOption(option, value)) { }
    else return false; // not used
    
    return true;
}

/*****************************************************/
void Options::Validate()
{
    if (mApiUrl.empty())
        throw MissingOptionException("apiurl");
}

} // namespace AndromedaCli
