#include <chrono>
#include <sstream>

#include "Options.hpp"

#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;

using namespace std::chrono;

namespace AndromedaFuse {

/*****************************************************/
std::string Options::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-fuse " << CoreBaseHelpText() << endl << endl
           
        << "Local Mount:     -m|--mountpath path" << endl
        << "Remote Endpoint: (-a|--apiurl url) | (-p|--apipath [path])" << endl << endl

        << "Remote Object:   [--folder [id] | --filesystem [id]]" << endl
        << "Remote Auth:     [-u|--username str] [--password str] | [--sessionid id] [--sessionkey key] [--force-session]" << endl << endl
       
        << HTTPOptions::HelpText() << endl
        << RunnerOptions::HelpText() << endl << endl
        << FuseOptions::HelpText() << endl << endl
        
        << ConfigOptions::HelpText() << endl
        << CacheOptions::HelpText() << endl << endl
           
        << OtherBaseHelpText() << endl;

    return output.str();
}

/*****************************************************/
Options::Options(ConfigOptions& configOptions, 
                 HTTPOptions& httpOptions,
                 RunnerOptions& runnerOptions,
                 CacheOptions& cacheOptions,
                 FuseOptions& fuseOptions) :
    mConfigOptions(configOptions), 
    mHttpOptions(httpOptions), 
    mRunnerOptions(runnerOptions),
    mCacheOptions(cacheOptions),
    mFuseOptions(fuseOptions) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (flag == "p" || flag == "apipath")
        mApiType = ApiType::API_PATH;

    else if (flag == "force-session")
        mForceSession = true;

    else if (flag == "filesystem")
        mMountRootType = RootType::FILESYSTEM;
    else if (flag == "folder")
        mMountRootType = RootType::FOLDER;

    else if (flag == "d" || flag == "debug")
        mForeground = true;
    
    else if (BaseOptions::AddFlag(flag)) { }
    else if (mConfigOptions.AddFlag(flag)) { }
    else if (mHttpOptions.AddFlag(flag)) { }
    else if (mRunnerOptions.AddFlag(flag)) { }
    else if (mCacheOptions.AddFlag(flag)) { }
    else if (mFuseOptions.AddFlag(flag)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    /** Backend endpoint selection */
    if (option == "a" || option == "apiurl")
    {
        mApiPath = value;
        mApiType = ApiType::API_URL;

        // Certain details can be parsed from the URL
        ParseUrl(mApiPath);
    }
    else if (option == "p" || option == "apipath")
    {
        mApiPath = value;
        mApiType = ApiType::API_PATH;
    }

    /** Backend authentication details */
    else if (option == "u" || option == "username")
        mUsername = value;
    else if (option == "password")
        mPassword = value;
    else if (option == "sessionid")
        mSessionid = value;
    else if (option == "sessionkey")
        mSessionkey = value;

    /** Backend mount object selection */
    else if (option == "ri" || option == "filesystem")
    {
        mMountItemID = value;
        mMountRootType = RootType::FILESYSTEM;
    }
    else if (option == "rf" || option == "folder")
    {
        mMountItemID = value;
        mMountRootType = RootType::FOLDER;
    }
    else if (option == "m" || option == "mountpath")
        mMountPath = value;

    else if (option == "d" || option == "debug")
    {
        mForeground = true;
        BaseOptions::AddOption(option, value);
    }

    else if (BaseOptions::AddOption(option, value)) { }
    else if (mConfigOptions.AddOption(option, value)) { }
    else if (mHttpOptions.AddOption(option, value)) { }
    else if (mRunnerOptions.AddOption(option, value)) { }
    else if (mCacheOptions.AddOption(option, value)) { }
    else if (mFuseOptions.AddOption(option, value)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
void Options::TryAddUrlOption(const std::string& option, const std::string& value)
{
    if (option == "folder")
    {
        mMountItemID = value;
        mMountRootType = RootType::FOLDER;
    }
}

/*****************************************************/
void Options::Validate()
{
    if (mApiType == static_cast<ApiType>(-1))
        throw MissingOptionException("apiurl/apipath");
    if (mMountPath.empty())
        throw MissingOptionException("mountpath");
}

} // namespace AndromedaFuse
