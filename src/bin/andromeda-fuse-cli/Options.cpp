#include <chrono>
#include <list>
#include <map>
#include <sstream>
#include <vector>

#include "Options.hpp"

#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

#include "andromeda/Config.hpp"
using Andromeda::Config;
#include "andromeda/HTTPRunner.hpp"
using Andromeda::HTTPRunner;
#include "andromeda/Utilities.hpp"
using Andromeda::Debug;
using Andromeda::Utilities;

using namespace std::chrono;

/*****************************************************/
std::string Options::HelpText()
{
    std::ostringstream output;

    Config::Options cfgDefault;
    HTTPRunner::Options httpDefault;

    const auto defRefresh(seconds(cfgDefault.refreshTime).count());
    const auto defRetry(seconds(httpDefault.retryTime).count());
    const auto defTimeout(seconds(httpDefault.timeout).count());

    using std::endl;

    output << "Usage Syntax: " << endl
           << "andromeda-fuse (-h|--help | -V|--version)" << endl << endl
           
           << "Local Mount:     -m|--mountpath path" << endl
           << "Remote Endpoint: (-s|--apiurl url) | (-p|--apipath [path])" << endl << endl

           << "Remote Object:   [--folder [id] | --filesystem [id]]" << endl
           << "Remote Auth:     [-u|--username str] [--password str] | [--sessionid id] [--sessionkey key] [--force-session]" << endl
           << "Permissions:     [-o uid=N] [-o gid=N] [-o umask=N] [-o allow_root] [-o allow_other] [-o default_permissions] [-r|--read-only]" << endl
           << "Advanced:        [-o fuseoption]+ [--pagesize bytes(" << cfgDefault.pageSize << ")] [--refresh secs(" << defRefresh << ")] [--no-chmod] [--no-chown]" << endl
           << "HTTP Options:    [--http-user str --http-pass str] [--proxy-host host [--proxy-port uint] [--hproxy-user str --hproxy-pass str]]" << endl
           << "HTTP Advanced:   [--http-timeout secs(" << defTimeout << ")] [--max-retries uint(" << httpDefault.maxRetries << ")] [--retry-time secs(" << defRetry << ")]" << endl
           << "Debugging:       [-d|--debug [uint]] [--cachemode none|memory|normal]" << endl;

    return output.str();
}

/*****************************************************/
Options::Options(Config::Options& configOptions, 
                 HTTPRunner::Options& httpOptions, 
                 FuseAdapter::Options& fuseOptions) : 
    mConfigOptions(configOptions), 
    mHttpOptions(httpOptions), 
    mFuseOptions(fuseOptions) { }

/*****************************************************/
void Options::ParseArgs(int argc, char** argv)
{
    Utilities::Flags flags;
    Utilities::Options options;

    if (!Utilities::parseArgs(argc, argv, flags, options))
        throw BadUsageException();

    LoadFrom(flags, options);
}

/*****************************************************/
void Options::ParseFile(const std::filesystem::path& path)
{
    Utilities::Flags flags;
    Utilities::Options options;

    Utilities::parseFile(path, flags, options);

    LoadFrom(flags, options);
}

/*****************************************************/
void Options::LoadFrom(const Utilities::Flags& flags, const Utilities::Options options)
{
    for (const std::string& flag : flags)
    {
        if (flag == "h" || flag == "help")
            throw ShowHelpException();
        else if (flag == "V" || flag == "version")
            throw ShowVersionException();

        else if (flag == "d" || flag == "debug")
            mDebugLevel = Debug::Level::ERRORS;
        else if (flag == "p" || flag == "apipath")
            mApiType = ApiType::API_PATH;

        else if (flag == "force-session")
            mForceSession = true;

        else if (flag == "filesystem")
            mMountRootType = RootType::FILESYSTEM;
        else if (flag == "folder")
            mMountRootType = RootType::FOLDER;

        else if (flag == "r" || flag == "read-only")
            mConfigOptions.readOnly = true;
        
        else if (flag == "no-chmod")
            mFuseOptions.fakeChmod = false;
        else if (flag == "no-chown")
            mFuseOptions.fakeChown = false;

        else throw BadFlagException(flag);
    }

    for (const decltype(options)::value_type& pair : options) 
    {
        const std::string& option { pair.first };
        const std::string& value { pair.second };

        if (option == "d" || option == "debug")
        {
            try { mDebugLevel = static_cast<Debug::Level>(stoul(value)); }
            catch (const std::logic_error& e) { 
                throw BadValueException(option); }
        }

        /** Backend endpoint selection */
        else if (option == "s" || option == "apiurl")
        {
            mApiPath = value;
            mApiType = ApiType::API_URL;

            Utilities::Flags urlFlags; 
            Utilities::Options urlOptions;

            Utilities::parseUrl(mApiPath, urlFlags, urlOptions);

            /** Certain mount details can be parsed from the URL */
            for (decltype(urlOptions)::value_type urlPair : urlOptions)
            {
                if (urlPair.first == "folder")
                {
                    mMountItemID = urlPair.second;
                    mMountRootType = RootType::FOLDER;
                }
            }
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

        /** FUSE wrapper options */
        else if (option == "m" || option == "mountpath")
        {
            mFuseOptions.mountPath = value;
        }
        else if (option == "o" || option == "option")
        {
            mFuseOptions.fuseArgs.push_back(value);
        }

        /** libandromeda Config options */
        else if (option == "cachemode")
        {
                 if (value == "none")   mConfigOptions.cacheType = Config::Options::CacheType::NONE;
            else if (value == "memory") mConfigOptions.cacheType = Config::Options::CacheType::MEMORY;
            else if (value == "normal") mConfigOptions.cacheType = Config::Options::CacheType::NORMAL;
            else throw BadValueException(option);
        }
        else if (option == "pagesize")
        {
            try { mConfigOptions.pageSize = stoul(value); }
            catch (const std::logic_error& e) { 
                throw BadValueException(option); }

            if (!mConfigOptions.pageSize) throw BadValueException(option);
        }
        else if (option == "refresh")
        {
            try { mConfigOptions.refreshTime = seconds(stoul(value)); }
            catch (const std::logic_error& e) { throw BadValueException(option); }
        }

        /** HTTP runner options */
        else if (option == "http-user")
            mHttpOptions.username = value;
        else if (option == "http-pass")
            mHttpOptions.password = value;
        else if (option == "proxy-host")
            mHttpOptions.proxyHost = value;
        else if (option == "proxy-port")
        {
            try { mHttpOptions.proxyPort = static_cast<decltype(mHttpOptions.proxyPort)>(stoul(value)); }
            catch (const std::logic_error& e) {
                throw BadValueException(option); }
        }
        else if (option == "hproxy-user")
            mHttpOptions.proxyUsername = value;
        else if (option == "hproxy-pass")
            mHttpOptions.proxyPassword = value;
        else if (option == "http-timeout")
        {
            try { mHttpOptions.timeout = seconds(stoul(value)); }
            catch (const std::logic_error& e) { throw BadValueException(option); }
        }
        else if (option == "max-retries")
        {
            try { mHttpOptions.maxRetries = stoul(value); }
            catch (const std::logic_error& e) {
                throw BadValueException(option); }
        }
        else if (option == "retry-time")
        {
            try { mHttpOptions.retryTime = seconds(stoul(value)); }
            catch (const std::logic_error& e) { throw BadValueException(option); }
        }

        else throw BadOptionException(option);
    }
}

/*****************************************************/
void Options::Validate()
{
    if (mApiType == static_cast<ApiType>(-1))
        throw MissingOptionException("apiurl/apipath");
    if (mFuseOptions.mountPath.empty())
        throw MissingOptionException("mountpath");
}
