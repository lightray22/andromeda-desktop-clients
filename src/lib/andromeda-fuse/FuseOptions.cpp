
#include <iostream>
#include <sstream>

#include "FuseOptions.hpp"
#include "libfuse_Includes.h"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;

namespace AndromedaFuse {

/*****************************************************/
std::string FuseOptions::HelpText()
{
    std::ostringstream output;
    FuseOptions optDefault;

    using std::endl;

    output << "FUSE Advanced:    [--no-chmod] [--no-chown] [--no-fuse-threading]"
    #if !LIBFUSE2
        << " [--fuse-max-idle-threads uint(" << optDefault.maxIdleThreads << ")]"
    #endif // !LIBFUSE2
        << " [-o fuseoption]+"; 
    
#if !LIBFUSE2
    output << " [--dump-fuse-options]" << endl;
#else // LIBFUSE2
    output << endl;
#endif // LIBFUSE2

    output << "FUSE Permissions: [--file_perm " << std::oct << optDefault.filePerms << "] [--dir_perm " << optDefault.dirPerms << "]"
        << " [-o uid=N] [-o gid=N] [-o umask=N] [-o allow_root] [-o allow_other]";

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
    else if (flag == "no-fuse-threading")
        enableThreading = false;
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
    else if (option == "file_perm")
    {
        if (option.size() != 3) throw BaseOptions::BadValueException(option);
        try { filePerms = decltype(filePerms)(stoul(value,nullptr,8)); }
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
    }
    else if (option == "dir_perm")
    {
        if (option.size() != 3) throw BaseOptions::BadValueException(option);
        try { dirPerms = decltype(dirPerms)(stoul(value,nullptr,8)); }
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
    }
#if !LIBFUSE2
    else if (option == "fuse-max-idle-threads")
    {
        try { maxIdleThreads = decltype(maxIdleThreads)(stoul(value)); }
        catch (const std::logic_error& e) { throw BaseOptions::BadValueException(option); }
    }
#endif // !LIBFUSE2
    else return false; // not used

    return true; 
}

} // namespace Andromeda
