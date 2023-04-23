
// a thin wrapper around andromeda-fuse supporting the "dev path [-o opts]" mount format
// any option in -o that starts with "-" goes to andromeda, otherwise goes to libfuse

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;

void PrintHelp()
{
    std::cout << "mount.andromeda usage: url|none path [-o fuseopt,--andromedaopt=val,+]" << std::endl
        << "e.g. mount.andromeda http://myserv /mnt -o ro,--no-chmod,-u=myuser" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 3) { 
        PrintHelp(); return 1; }

    std::string cmd("andromeda-fuse -q");
    
    if (!strcmp(argv[1],"none"))
        { cmd += " -a "; cmd += argv[1]; }
    cmd += " -m "; cmd += argv[2];

    if (argc > 3)
    {
        if (argc != 5 || strcmp(argv[3],"-o")) { 
            PrintHelp(); return 1; }

        for (const std::string& arg : Utilities::explode(argv[4],","))
        {
            if (!arg.size()) continue;
            if (arg[0] == '-') cmd += " "+arg;
            else cmd += " -o "+arg;
        }
    }

    return std::system(cmd.c_str());
}
