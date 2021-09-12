#include "Options.hpp"

#include <map>
#include <list>
#include <string>

using namespace std;

string Options::HelpText()
{
    ostringstream output;

    output << "Usage: a2fuse [-d|--debug int] [-u|--username username]" << endl
           << "\t((-s|--apiurl url) | (-p|--apipath path))" << endl;

    return output.str();
}

void Options::Initialize(int argc, char** argv)
{
    list<string> flags;
    map<string,string> options;

    for (size_t i = 1; i < argc; i++)
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
        if (flag == "d" || flag == "debug")
            debug = DebugLevel::DEBUG_BASIC;

        else throw BadFlagException(flag);
    }

    for (map<string,string>::iterator it = options.begin(); 
        it != options.end(); it++)
    {
        const string& option = it->first; 
        const string& value = it->second;

        if (option == "d" || option == "-debug")
        {
            try { this->debug = (DebugLevel)stoi(value); }
            catch (const logic_error& e) { 
                throw BadValueException(option); }
        }
        else if (option == "u" || option == "-username")
        {
            this->username = value;
        }
        else if (option == "s" || option == "-apiurl")
        {
            this->apiLoc = value;
            this->apiType = ApiType::API_URL;
        }
        else if (option == "p" || option == "-apipath")
        {
            this->apiLoc = value;
            this->apiType = ApiType::API_PATH;
        }

        else throw BadOptionException(option);
    }

    if (this->apiLoc.empty())
        throw MissingOptionException("apiurl/apipath");
}
