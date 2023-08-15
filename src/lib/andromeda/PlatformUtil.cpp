
#include <cstring>
#include <iostream>
#include <mutex>

// SilentReadConsole()
#if WIN32
#include <windows.h>
#else // !WIN32
#include <termios.h>
#endif // WIN32

// GetEnvironment()
#if WIN32
#include <processenv.h>
#else // !WIN32
#include <unistd.h>
#ifndef _GNU_SOURCE
extern char** environ;
#endif
#endif // WIN32

#include "PlatformUtil.hpp"
#include "StringUtil.hpp"

namespace Andromeda {

/*****************************************************/
void PlatformUtil::SilentReadConsole(std::string& retval)
{
#if WIN32
    HANDLE hStdin { GetStdHandle(STD_INPUT_HANDLE) }; 
    DWORD mode { 0 }; GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#else // !WIN32
    struct termios oflags { };
    tcgetattr(fileno(stdin), &oflags);

    struct termios nflags { oflags };
    nflags.c_lflag &= ~static_cast<decltype(nflags.c_lflag)>(ECHO); // -Wsign-conversion
    tcsetattr(fileno(stdin), TCSANOW, &nflags);
#endif // WIN32

    std::getline(std::cin, retval);

#if WIN32
    SetConsoleMode(hStdin, mode);
#else // !WIN32
    tcsetattr(fileno(stdin), TCSANOW, &oflags);
#endif // WIN32

    std::cout << std::endl;
}

/*****************************************************/
PlatformUtil::StringMap PlatformUtil::GetEnvironment(const std::string& prefix)
{
    StringMap retval;

#if WIN32
    LPCH env { GetEnvironmentStrings() };
    for (LPCTSTR var = env; var && var[0]; var += lstrlen(var)+1)
    {
#else // !WIN32
    for (const char* const* env { environ }; env && *env; ++env)
    {
        const char* var { *env };
#endif // WIN32

        StringUtil::StringPair pair { StringUtil::split(var, "=") }; // non-const for move
        if (prefix.empty() || StringUtil::startsWith(pair.first,prefix))
            retval.emplace(std::move(pair));
    }

#if WIN32
    FreeEnvironmentStrings(env);
#endif // WIN32

    return retval;
}

/*****************************************************/
std::string PlatformUtil::GetHomeDirectory()
{
    #if WIN32
        #pragma warning(push)
        #pragma warning(disable:4996) // getenv is safe in C++11
    #endif // WIN32

    for (const char* env : { "HOME", "HOMEDIR", "HOMEPATH" })
    {
        const char* path { std::getenv(env) }; // NOLINT(concurrency-mt-unsafe)
        if (path != nullptr) return path;
    }

    #if WIN32
        #pragma warning(pop)
    #endif // WIN32

    return ""; // not found
}

// mutex protecting std::strerror
std::mutex sStrerrorMutex;

/*****************************************************/
std::string PlatformUtil::GetErrorString(int err)
{
    const std::lock_guard<std::mutex> llock(sStrerrorMutex);

    #if WIN32
        #pragma warning(push)
        #pragma warning(disable:4996) // we lock strerror
    #endif // WIN32

    return std::string(std::strerror(err)); // NOLINT(concurrency-mt-unsafe)

    #if WIN32
        #pragma warning(pop)
    #endif // WIN32
}

} // namespace Andromeda
