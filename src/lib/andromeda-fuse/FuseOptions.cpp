
#include <iostream>
#include <sstream>

#include "FuseOptions.hpp"
#include "libfuse_Includes.h"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;

namespace AndromedaFuse {

/*****************************************************/
std::string FuseOptions::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output << "FUSE Advanced:    [--no-chmod] [--no-chown] [-o fuseoption]+"; 
    
#if !LIBFUSE2
    output << " [--dump-fuse-options]" << endl;
#else // LIBFUSE2
    output << endl;
#endif // LIBFUSE2

    output << "FUSE Permissions: [-o uid=N] [-o gid=N] [-o umask=N] [-o allow_root] [-o allow_other] [-o default_permissions]";

    return output.str();
}

#if !LIBFUSE2
/*****************************************************/
void FuseOptions::ShowFuseHelpText()
{ 
    std::cout << "Advanced FUSE options:" << std::endl;
    fuse_lib_help(nullptr); std::cout << std::endl; 
}
#endif // LIBFUSE2

/*****************************************************/
bool FuseOptions::AddFlag(const std::string& flag)
{
    if (flag == "no-chmod")
        fakeChmod = false;
    else if (flag == "no-chown")
        fakeChown = false;
#if !LIBFUSE2
    else if (flag == "dump-fuse-options")
    {
        ShowFuseHelpText();
        throw BaseOptions::ShowHelpException();
    }
#endif // LIBFUSE2
    else return false; // not used

    return true;
}

/*****************************************************/
bool FuseOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "o" || option == "option")
    {
        fuseArgs.push_back(value);
    }
    else return false; // not used

    return true; 
}

} // namespace Andromeda
