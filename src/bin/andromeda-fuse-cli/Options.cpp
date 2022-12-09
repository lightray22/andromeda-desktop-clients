#include <chrono>
#include <sstream>

#include "Options.hpp"

#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/backend/ConfigOptions.hpp"
using Andromeda::Backend::ConfigOptions;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;

using namespace std::chrono;

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
       
        << HTTPOptions::HelpText() << endl << endl
        << FuseOptions::HelpText() << endl << endl
        << ConfigOptions::HelpText() << endl << endl
           
        << OtherBaseHelpText() << endl;

    return output.str();
}

/*****************************************************/
Options::Options(ConfigOptions& configOptions, 
                 HTTPOptions& httpOptions, 
                 FuseOptions& fuseOptions) :
    mConfigOptions(configOptions), 
    mHttpOptions(httpOptions), 
    mFuseOptions(fuseOptions) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (BaseOptions::AddFlag(flag)) { }

    else if (flag == "p" || flag == "apipath")
        mApiType = ApiType::API_PATH;

    else if (flag == "force-session")
        mForceSession = true;

    else if (flag == "filesystem")
        mMountRootType = RootType::FILESYSTEM;
    else if (flag == "folder")
        mMountRootType = RootType::FOLDER;
    
    else if (mConfigOptions.AddFlag(flag)) { }
    else if (mHttpOptions.AddFlag(flag)) { }
    else if (mFuseOptions.AddFlag(flag)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    if (BaseOptions::AddOption(option, value)) { }

    /** Backend endpoint selection */
    else if (option == "a" || option == "apiurl")
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
    {
        mMountPath = value;
    }

    else if (mConfigOptions.AddOption(option, value)) { }
    else if (mHttpOptions.AddOption(option, value)) { }
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
