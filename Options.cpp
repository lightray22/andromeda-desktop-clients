#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <chrono>

#include "Options.hpp"
#include "Utilities.hpp"

using namespace std;

/*****************************************************/
string Options::HelpText()
{
    ostringstream output;

    Config::Options defOptions;

    const int defRefresh(std::chrono::seconds(defOptions.refreshTime).count());

    output << "Usage Syntax: " << endl
           << "andromeda-fuse (-h|--help | -V|--version)" << endl << endl
           
           << "Local Mount:     -m|--mount path" << endl
           << "Remote Endpoint: (-s|--apiurl url) | (-p|--apipath path)" << endl
           << "Remote Object:   [(-rf|--folder [id]) | (-ri|--filesystem [id])]" << endl
           << "Remote Options:  [-u|--username username] [-ro|--read-only]" << endl
           << "Permissions:     [-o uid=N] [-o gid=N] [-o umask=N] [-o allow_root] [-o allow_other] [-o default_permissions]" << endl
           << "Advanced:        [-o fuseoption]+ [--pagesize bytes(" << defOptions.pageSize << ")] [--refresh secs(" << defRefresh << ")] [--no-chmod] [--no-chown]" << endl
           << "Debugging:       [-d|--debug [int]] [--cachemode none|memory|normal]" << endl;

    return output.str();
}

/*****************************************************/
void Options::Parse(int argc, char** argv, Config::Options& opts)
{
    Utilities::Flags flags;
    Utilities::Options options;

    if (!Utilities::parseArgs(argc, argv, flags, options))
        throw BadUsageException();

    for (const string& flag : flags)
    {
        if (flag == "h" || flag == "-help")
            throw ShowHelpException();
        else if (flag == "V" || flag == "-version")
            throw ShowVersionException();

        else if (flag == "d" || flag == "-debug")
            this->debugLevel = Debug::Level::ERRORS;

        else if (flag == "ri" || flag == "-filesystem")
            this->mountItemType = ItemType::FILESYSTEM;
        else if (flag == "rf" || flag == "-folder")
            this->mountItemType = ItemType::FOLDER;

        else if (flag == "ro")
            opts.readOnly = true;
        else if (flag == "-no-chmod")
            this->fakeChmod = false;
        else if (flag == "-no-chown")
            this->fakeChown = false;

        else throw BadFlagException(flag);
    }

    for (const decltype(options)::value_type& pair : options) 
    {
        const string& option = pair.first;
        const string& value = pair.second;

        if (option == "d" || option == "-debug")
        {
            try { this->debugLevel = static_cast<Debug::Level>(stoi(value)); }
            catch (const logic_error& e) { 
                throw BadValueException(option); }
        }
        else if (option == "s" || option == "-apiurl")
        {
            vector<string> parts = 
                Utilities::explode(value, "/", 2, 2);

            if (parts.size() != 2) 
                throw BadValueException(option);

            this->apiPath = "/"+parts[1];
            this->apiHostname = parts[0];
            this->apiType = ApiType::API_URL;
        }
        else if (option == "p" || option == "-apipath")
        {
            this->apiPath = value;
            this->apiType = ApiType::API_PATH;
        }
        else if (option == "u" || option == "-username")
        {
            this->username = value;
        }
        else if (option == "ri" || option == "-filesystem")
        {
            this->mountItemID = value;
            this->mountItemType = ItemType::FILESYSTEM;
        }
        else if (option == "rf" || option == "-folder")
        {
            this->mountItemID = value;
            this->mountItemType = ItemType::FOLDER;
        }
        else if (option == "m" || option == "-mountpath")
        {
            this->mountPath = value;
        }
        else if (option == "o" || option == "-option")
        {
            this->fuseOptions.push_back(value);
        }
        else if (option == "-cachemode")
        {
                 if (value == "none")   opts.cacheType = Config::Options::CacheType::NONE;
            else if (value == "memory") opts.cacheType = Config::Options::CacheType::MEMORY;
            else if (value == "normal") opts.cacheType = Config::Options::CacheType::NORMAL;
            else throw BadValueException(option);
        }
        else if (option == "-pagesize")
        {
            try { opts.pageSize = stoi(value); }
            catch (const logic_error& e) { 
                throw BadValueException(option); }

            if (!opts.pageSize) throw BadValueException(option);
        }
        else if (option == "-refresh")
        {
            try { opts.refreshTime = std::chrono::seconds(stoi(value)); }
            catch (const logic_error& e) { throw BadValueException(option); }
        }
        else throw BadOptionException(option);
    }

    if (this->apiPath.empty())
        throw MissingOptionException("apiurl/apipath");
    if (this->mountPath.empty())
        throw MissingOptionException("mountpath");
}
