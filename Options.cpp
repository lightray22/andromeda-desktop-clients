#include <map>
#include <list>
#include <vector>
#include <sstream>

#include "Options.hpp"
#include "andromeda/Utilities.hpp"

using namespace std;

/*****************************************************/
string Options::HelpText()
{
    ostringstream output;

    output << "Usage Syntax: " << endl
           << "andromeda-fuse (-h|--help | -V|--version)" << endl
           << "andromeda-fuse ((-s|--apiurl url) | (-p|--apipath path)) -m|--mount path" << endl
               << "\t[-d|--debug [int]] [-u|--username username] [-o fuseoption]+" << endl
               << "\t[(-rf|--folder [id]) | (-ri|--filesystem [id]) | (-rs|--share [id])]" << endl;

    return output.str();
}

/*****************************************************/
void Options::Parse(int argc, char** argv)
{
    list<string> flags;
    map<string,string> options;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
            throw BadUsageException();

        const char* flag = argv[i]+1;
        if (argc-1 > i && argv[i+1][0] != '-')
            options[flag] = argv[++i];
        else flags.push_back(flag);
    }

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

        else throw BadFlagException(flag);
    }

    for (const map<string,string>::value_type& pair : options) 
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
                throw BadValueException("apiurl");

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
        else throw BadOptionException(option);
    }

    if (this->apiPath.empty())
        throw MissingOptionException("apiurl/apipath");
    if (this->mountPath.empty())
        throw MissingOptionException("mountpath");
}
