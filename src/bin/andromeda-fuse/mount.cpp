
// a thin wrapper around andromeda-fuse supporting the "dev path [-o opts]" mount format
// any option in -o that starts with "-" goes to andromeda, otherwise goes to libfuse

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;

/** Print help text to std::cout */
void PrintHelp()
{
    using std::endl;
    std::cout << "usage: mount.andromeda url|none path [-o fuseopt,--andromedaopt=val,+]" << endl
        << "... if url is \"none\" then no url will be passed (e.g. to use a CLI path instead)" << endl
        << "... \"mount -t andromeda\" can be used to call mount.andromeda if installed" << endl << endl;
    
    std::cout << "example: mount.andromeda http://myserv /mnt -o ro,--no-chmod,-u=myuser" << endl
        << "example fstab: http://myserv /mnt andromeda ro,--no-chmod,-u=myuser 0 0" << endl;
}

int main(int argc, char** argv)
{
    if (argc < 3) { 
        PrintHelp(); return 1; }

    std::string cmd("andromeda-fuse -q"); // quiet
    
    if (!strcmp(argv[1],"none"))
        { cmd += " -a "; cmd += Utilities::quoteString(argv[1]); } // url
    cmd += " -m "; cmd += Utilities::quoteString(argv[2]); // path

    if (argc > 3) // -o is optional
    {
        if (argc != 5 || strcmp(argv[3],"-o") != 0) {
            PrintHelp(); return 1; }

        for (const std::string& arg : Utilities::explode(argv[4],","))
        {
            if (arg.empty()) continue; // invalid

            if (arg[0] == '-') 
                cmd += " "+Utilities::quoteString(arg); // andromeda option
            else cmd += " -o "+Utilities::quoteString(arg); // libfuse option
        }
    }

    //std::cout << cmd << std::endl; // debug
    return std::system(cmd.c_str()); // NOLINT(cert-env33-c) // NOLINT(concurrency-mt-unsafe)
}
