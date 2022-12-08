
#include <algorithm>
#include <iostream>

#if WIN32
#include <windows.h>
#else // !WIN32
#include <termios.h>
#endif // WIN32

#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
Utilities::StringList Utilities::explode(
    std::string str, const std::string& delim, 
    const size_t skip, const bool reverse, const size_t max)
{
    StringList retval;
    
    if (str.empty()) return retval;
    if (delim.empty()) return { str };

    std::string segment; size_t skipped { 0 };

    if (reverse) std::reverse(str.begin(), str.end());

    while ( retval.size() + 1 < max )
    {
        const size_t segEnd { str.find(delim) };
        if (segEnd == std::string::npos) break;

        segment += str.substr(0, segEnd);

        if (skipped >= skip)
        { 
            retval.push_back(segment); 
            segment.clear(); 
        }
        else { skipped++; segment += delim; }

        str.erase(0, segEnd + delim.length());
    }

    retval.push_back(segment+str);

    if (reverse)
    {
        for (std::string& el : retval)
            std::reverse(el.begin(), el.end());
        std::reverse(retval.begin(), retval.end());
    }

    return retval;
}

/*****************************************************/
Utilities::StringPair Utilities::split(
    const std::string& str, const std::string& delim, 
    const size_t skip, const bool reverse)
{
    Utilities::StringList list { explode(str, delim, skip, reverse, 2) };

    if (list.size() < 1) list.push_back("");
    if (list.size() < 2) list.push_back("");

    return Utilities::StringPair { list[0], list[1] };
}

/*****************************************************/
bool Utilities::startsWith(const std::string& str, const std::string& start)
{
    if (start.size() > str.size()) return false;

    return std::equal(start.begin(), start.end(), str.begin());
}

/*****************************************************/
bool Utilities::endsWith(const std::string& str, const std::string& end)
{
    if (end.size() > str.size()) return false;

    return std::equal(end.rbegin(), end.rend(), str.rbegin());
}

/*****************************************************/
bool Utilities::stringToBool(std::string str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), 
        [](unsigned char c) { return !std::isspace(c); }
    ));

    return (str != "" && str != "0" && str != "false" && str != "off" && str != "no");
}

/*****************************************************/
void Utilities::SilentReadConsole(std::string& retval)
{
    #if WIN32
        HANDLE hStdin { GetStdHandle(STD_INPUT_HANDLE) }; 
        DWORD mode { 0 }; GetConsoleMode(hStdin, &mode);
        SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
    #else // !WIN32
        struct termios oflags, nflags;
        tcgetattr(fileno(stdin), &oflags);

        nflags = oflags;
        nflags.c_lflag &= ~static_cast<decltype(nflags.c_lflag)>(ECHO); // -Wsign-conversion
        tcsetattr(fileno(stdin), TCSANOW, &nflags);
    #endif

    std::getline(std::cin, retval);
    
    #if WIN32
        SetConsoleMode(hStdin, mode);
    #else // !WIN32
        tcsetattr(fileno(stdin), TCSANOW, &oflags);
    #endif

    std::cout << std::endl;
}

} // namespace Andromeda
