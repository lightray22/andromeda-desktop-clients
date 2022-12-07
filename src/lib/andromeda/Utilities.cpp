#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#if WIN32
#include <windows.h>
#else // !WIN32
#include <termios.h>
#endif // WIN32

#include "Utilities.hpp"

using namespace std::chrono;

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
bool Utilities::endsWith(const std::string& str, const std::string& end)
{
    if (end.size() > str.size()) return false;

    return std::equal(end.rbegin(), end.rend(), str.rbegin());
}

/*****************************************************/
bool Utilities::parseArgs(int argc, char** argv, 
    Utilities::Flags& flags, Utilities::Options& options)
{
    for (int i { 1 }; i < argc; i++)
    {
        if (strlen(argv[i]) < 2 || argv[i][0] != '-') return false;

        const char* flag { argv[i]+1 };
        bool ext { (flag[0] == '-') };
        if (ext) flag++; // --opt

        if (!ext && strlen(flag) > 1)
            options.emplace(std::string(1,flag[0]), flag+1); // -x3

        else if (argc-1 > i && argv[i+1][0] != '-')
            options.emplace(flag, argv[++i]); // -x 3

        else flags.push_back(flag); // -x
    }

    return true;
}

/*****************************************************/
void Utilities::parseFile(const std::filesystem::path& path, 
    Utilities::Flags& flags, Utilities::Options& options)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    while (file.good())
    {
        std::string line; std::getline(file,line);

        if (!line.size() || line.at(0) == '#' || line.at(0) == ' ') continue;

        StringPair pair(split(line, "="));

        if (pair.second.size()) 
             options.emplace(pair);
        else flags.push_back(pair.first);
    }
}

/*****************************************************/
void Utilities::parseUrl(const std::string& url, 
    Utilities::Flags& flags, Utilities::Options& options)
{
    const size_t sep(url.find("?"));

    if (sep != std::string::npos)
    {
        const std::string& substr(url.substr(sep+1));

        for (const std::string& param : explode(substr,"&"))
        {
            StringPair pair(split(param,"="));
            
            if (pair.second.size()) 
                 options.emplace(pair);
            else flags.push_back(pair.first);
        }
    }
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

std::mutex Debug::sMutex;
Debug::Level Debug::sLevel { Debug::Level::NONE };
high_resolution_clock::time_point Debug::sStart { high_resolution_clock::now() };

/*****************************************************/
Debug::operator bool() const
{
    return static_cast<bool>(sLevel);
}

/*****************************************************/
void Debug::Info(const std::string& str)
{
    if (sLevel >= Level::INFO) Error(str);

    if (str.empty()) mBuffer.str(std::string());
}

/*****************************************************/
void Debug::Backend(const std::string& str)
{
    if (sLevel >= Level::BACKEND) Error(str);

    if (str.empty()) mBuffer.str(std::string());
}

/*****************************************************/
void Debug::Error(const std::string& str)
{
    if (sLevel < Level::ERRORS) return;

    const std::lock_guard<std::mutex> lock(sMutex);

    if (sLevel >= Level::DETAILS)
    {
        duration<double> time { high_resolution_clock::now() - sStart };
        std::cerr << "time:" << time.count() << " ";

        std::cerr << "tid:" << std::this_thread::get_id() << " ";

        if (mAddr != nullptr) std::cerr << "obj:" << mAddr << " ";
    }

    std::cerr << mPrefix << ": ";

    if (str.empty())
    {
        std::cerr << mBuffer.str() << std::endl; 
        
        mBuffer.str(std::string()); // reset buffer
    }
    else std::cerr << str << std::endl;
}

/*****************************************************/
void Debug::DumpBytes(const void* ptr, uint64_t bytes, uint8_t width)
{
    mBuffer << "printing " << bytes << " bytes at " 
        << std::hex << ptr << std::endl;

    for (decltype(bytes) i { 0 }; i < bytes; i++)
    {
        const uint8_t* byte { static_cast<const uint8_t*>(ptr)+i };
        
        if (i % width == 0) mBuffer << static_cast<const void*>(byte) << ": ";
        
        // need to cast to a 16-bit integer so it gets printed as a number not a character
        mBuffer << std::setw(2) << std::setfill('0') << static_cast<uint16_t>(*byte) << " ";

        if ((i % width) + 1 == width) mBuffer << std::endl;
    }

    mBuffer << std::endl;
}

} // namespace Andromeda
