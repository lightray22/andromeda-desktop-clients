
#include <iostream>
#include <sstream>

#include "FuseOptions.hpp"
#include "libfuse_Includes.h"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;

namespace AndromedaFuse {

/*****************************************************/
std::string FuseOptions::HelpText()
{
    std::ostringstream output;
    const FuseOptions optDefault;

    using std::endl;

    output << "FUSE Advanced:    [--no-chmod] [--no-chown]"
    #ifndef OPENBSD
        << " [--no-fuse-threading]"
    #endif // !OPENBSD
    #if !LIBFUSE2
        << " [--fuse-max-idle-threads uint32(" << optDefault.maxIdleThreads << ")]"
    #endif // !LIBFUSE2
        << " [-o fuseoption]+"; 
    
#if !LIBFUSE2
    output << " [--dump-fuse-options]" << endl;
#else // LIBFUSE2
    output << endl;
#endif // LIBFUSE2

    output << "FUSE Permissions: [--file-mode " << std::oct << optDefault.fileMode << "] [--dir-mode " << optDefault.dirMode << "]"
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
#ifndef OPENBSD
    else if (flag == "no-fuse-threading")
        enableThreading = false;
#endif // !OPENBSD
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
    else if (option == "file-mode")
    {
        if (value.size() != 3) throw BaseOptions::BadValueException(option);
        try { fileMode = static_cast<decltype(fileMode)>(stoul(value,nullptr,8)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
    else if (option == "dir-mode")
    {
        if (value.size() != 3) throw BaseOptions::BadValueException(option);
        try { dirMode = static_cast<decltype(dirMode)>(stoul(value,nullptr,8)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
#if !LIBFUSE2
    else if (option == "fuse-max-idle-threads")
    {
        try { maxIdleThreads = static_cast<decltype(maxIdleThreads)>(stoul(value)); }
        catch (const std::logic_error& e) { 
            throw BaseOptions::BadValueException(option); }
    }
#endif // !LIBFUSE2
    else return false; // not used

    return true; 
}

} // namespace AndromedaFuse
