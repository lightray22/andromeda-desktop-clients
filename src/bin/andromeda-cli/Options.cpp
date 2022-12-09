
#include <sstream>

#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;

/*****************************************************/
std::string Options::CoreHelpText()
{
    return CoreBaseHelpText();
}

/*****************************************************/
std::string Options::OtherHelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << HTTPOptions::HelpText() << endl << endl
           
        << OtherBaseHelpText();

    return output.str();
}

/*****************************************************/
Options::Options(HTTPOptions& httpOptions) :
    mHttpOptions(httpOptions) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (BaseOptions::AddFlag(flag)) { }
    else if (mHttpOptions.AddFlag(flag)) { }
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
    else return false; // not used
    
    return true;
}

/*****************************************************/
void Options::Validate()
{
    if (mApiUrl.empty())
        throw MissingOptionException("apiurl");
}
