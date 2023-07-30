
// a thin wrapper around andromeda-fuse supporting the "dev path [-o opts]" mount format
// any option in -o that starts with "-" goes to andromeda, otherwise goes to libfuse

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;
#include "andromeda/backend/CLIRunner.hpp"
using Andromeda::Backend::CLIRunner;

/** Print help text to std::cout */
void PrintHelp()
{
    using std::endl;
    std::cout << "usage: mount.andromeda url|none path [-o fuseopt,--andromedaopt=val,...]" << endl
        << "... if url is \"none\" then no url will be passed (e.g. to use a CLI path instead)" << endl
        << "... \"mount -t andromeda\" can be used to call mount.andromeda if installed" << endl << endl;
    
    std::cout 
        << "example (manual):    mount.andromeda http://myserv /mnt -o ro,--no-chmod,-u=myuser" << endl
        << "example (use fstab): mount /mnt -o ro,--no-chmod,-u=myuser" << endl
        << "example fstab line:  http://myserv /mnt andromeda ro,--no-chmod,-u=myuser 0 0" << endl;
}

int main(int argc, char** argv)
{
    if (argc < 3) { 
        PrintHelp(); return 1; }

    CLIRunner::ArgList args { "andromeda-fuse", "-q" }; // quiet
    
    if (!strcmp(argv[1],"none"))
        { args.emplace_back("-a"); args.emplace_back(argv[1]); } // apiurl

    args.emplace_back("-m"); args.emplace_back(argv[2]); // mountpath

    if (argc > 3) // -o is optional
    {
        if (argc != 5 || strcmp(argv[3],"-o") != 0) {
            PrintHelp(); return 1; }

        for (const std::string& arg : StringUtil::explode(argv[4],","))
        {
            if (arg.empty()) continue; // invalid

            if (arg[0] == '-') args.emplace_back(arg); // andromeda option
            else { args.emplace_back("-o"); args.emplace_back(arg); } // libfuse option
        }
    }

    try { return CLIRunner::RunPosixCommand(args); }
    catch (const CLIRunner::CmdException& ex){ 
        std::cerr << ex.what() << std::endl; return 2; }
}
