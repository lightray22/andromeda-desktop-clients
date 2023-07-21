
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <locale>
#include <mutex>
#include <random>

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

#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
std::string Utilities::Random(const size_t size)
{
    static const char chars[] = "0123456789abcdefghijkmnopqrstuvwxyz_"; // NOLINT(*-avoid-c-arrays)
    std::default_random_engine rng(std::random_device{}());

    // set the max to chars-2 as chars has a NUL term, and dist includes max
    std::uniform_int_distribution<> dist(0, sizeof(chars)-2);

    std::string retval; retval.resize(size);
    for (size_t i { 0 }; i < size; ++i)
        retval[i] = chars[dist(rng)];
    return retval;
}

/*****************************************************/
Utilities::StringList Utilities::explode(
    std::string str, const std::string& delim, 
    const size_t skip, const bool reverse, const size_t max)
{
    StringList retval;
    
    if (str.empty()) return retval;
    if (delim.empty()) return { str };

    if (reverse) std::reverse(str.begin(), str.end());

    { // variable scope
        std::string el;
        size_t skipped { 0 };

        while ( retval.size() + 1 < max )
        {
            const size_t segEnd { str.find(delim) };
            if (segEnd == std::string::npos) break;

            el += str.substr(0, segEnd);

            if (skipped >= skip)
            {
                retval.push_back(el);
                el.clear();
            }
            else { ++skipped; el += delim; }

            str.erase(0, segEnd + delim.length());
        }

        retval.push_back(el+str);
    }

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

    while (list.size() < 2)
    {
        if (!reverse) list.emplace_back("");
        else list.insert(list.begin(),"");
    }

    return Utilities::StringPair { list[0], list[1] };
}

/*****************************************************/
Utilities::StringPair Utilities::splitPath(const std::string& str)
{
    // TODO does this work if str ends with / ? why own function?
    return split(str,"/",0,true);
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
void Utilities::trim(std::string& str)
{
    const size_t size { str.size() };

    size_t start = 0; while (start < size && std::isspace(str[start])) ++start;
    size_t end = size; while (end > 0 && std::isspace(str[end-1])) --end;

    str.erase(end); str.erase(0, start);
}

/*****************************************************/
std::string Utilities::trim(const std::string& str)
{
    const size_t size { str.size() };

    size_t start = 0; while (start < size && std::isspace(str[start])) ++start;
    size_t end = size; while (end > 0 && std::isspace(str[end-1])) --end;

    return str.substr(start, end-start);
}

/*****************************************************/
std::string Utilities::tolower(const std::string& str)
{
    std::string ret(str);
    std::transform(ret.begin(), ret.end(), ret.begin(),
        [](char c)->char{ return std::tolower(c, std::locale()); });
    return ret;
}

/*****************************************************/
std::string Utilities::replaceAll(const std::string& str, const std::string& from, const std::string& repl)
{
    if (str.empty() || from.empty()) return str; // invalid
    std::string retval(str); // copy

    for (size_t pos = 0; (pos = retval.find(from, pos)) != std::string::npos; pos += repl.size())
        retval.replace(pos, from.size(), repl);
    return retval;
}

/*****************************************************/
bool Utilities::stringToBool(const std::string& stri)
{
    const std::string str { trim(stri) };
    return (!str.empty() && str != "0" && str != "false" && str != "off" && str != "no");
}

static constexpr size_t bytesMul { 1024 };

/*****************************************************/
uint64_t Utilities::stringToBytes(const std::string& stri)
{
    std::string str { trim(stri) };
    if (str.empty()) return 0;

    const char unit { str.at(str.size()-1) };
    
    if (unit < '0' || unit > '9')
    {
        str.pop_back(); trim(str);
        if (str.empty()) return 0;
    }

    // stoul throws std::logic_error
    uint64_t num { stoul(str) };

    switch (unit)
    {
        case 'P': num *= bytesMul;
        [[fallthrough]];
        case 'T': num *= bytesMul;
        [[fallthrough]];
        case 'G': num *= bytesMul;
        [[fallthrough]];
        case 'M': num *= bytesMul;
        [[fallthrough]];
        case 'K': num *= bytesMul;
        [[fallthrough]];
        default: break; // invalid
    }

    return num;
}

/*****************************************************/
std::string Utilities::bytesToString(uint64_t bytes)
{
    size_t unit { 0 };
    static const std::array<const char*,6> units { "", "K", "M", "G", "T", "P" };
    while (bytes >= bytesMul && !(bytes % bytesMul) && unit < units.size()-1) {
        ++unit; bytes /= bytesMul;
    }
    return std::to_string(bytes)+units[unit];
}

/*****************************************************/
void Utilities::SilentReadConsole(std::string& retval)
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
Utilities::StringMap Utilities::GetEnvironment(const std::string& prefix)
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

        StringPair pair { split(var, "=") }; // non-const for move
        if (prefix.empty() || startsWith(pair.first,prefix))
            retval.emplace(std::move(pair));
    }

#if WIN32
    FreeEnvironmentStrings(env);
#endif // WIN32

    return retval;
}

/*****************************************************/
std::string Utilities::GetHomeDirectory()
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
std::string Utilities::GetErrorString(int err)
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
