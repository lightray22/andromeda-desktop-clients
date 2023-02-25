
#include <sstream>

#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;

namespace AndromedaGui {

/*****************************************************/
std::string Options::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-gui " << CoreBaseHelpText() << endl << endl
           
        << CacheOptions::HelpText() << endl << endl

        << OtherBaseHelpText() << endl;

    return output.str();
}

/*****************************************************/
Options::Options(CacheOptions& cacheOptions) :
    mCacheOptions(cacheOptions) { }


/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (BaseOptions::AddFlag(flag)) { }
    else if (mCacheOptions.AddFlag(flag)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    if (BaseOptions::AddOption(option, value)) { }
    else if (mCacheOptions.AddOption(option, value)) { }

    else return false; // not used
    
    return true;
}

} // namespace AndromedaGui
