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
    const int max, const size_t skip)
{
    StringList retval;
    
    if (str.empty()) return retval;

    size_t skipped = 0, start = 0, end;
    while ((end = str.find(delim, start)) != std::string::npos
            && (max < 0 || retval.size()+1 < (size_t)max))
    {
        if (skipped >= skip)
        {
            start = 0;
            retval.push_back(str.substr(0, end));
            str.erase(0, end + delim.length());
        }
        else 
        {
            skipped++;
            start = (end + delim.length());
        }
    }

    retval.push_back(str);
    return retval;
}

/*****************************************************/
Utilities::StringPair Utilities::split(
    const std::string& str, const std::string& delim, const bool last)
{
    StringPair retval;

    size_t pos = last ? str.rfind(delim) : str.find(delim);

    if (pos == std::string::npos)
    {
        if (last) 
             retval.second = str;
        else retval.first = str;
    }
    else
    {
        retval.first = str.substr(0, pos);
        retval.second = str.substr(pos + delim.length());
    }

    return retval;
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
    for (int i = 1; i < argc; i++)
    {
        if (strlen(argv[i]) < 2 || argv[i][0] != '-') return false;

        const char* flag = argv[i]+1;
        bool ext = (flag[0] == '-');
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
bool Utilities::stringToBool(const std::string& str)
{
    return (str != "" && str != "0" && str != "false" && str != "off" && str != "no");
}

/*****************************************************/
void Utilities::SilentReadConsole(std::string& retval)
{
    #if WIN32
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
        DWORD mode = 0; GetConsoleMode(hStdin, &mode);
        SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
    #else // !WIN32
        struct termios oflags, nflags;
        tcgetattr(fileno(stdin), &oflags);

        nflags = oflags;
        nflags.c_lflag &= ~ECHO;
        nflags.c_lflag |= ECHONL;
        tcsetattr(fileno(stdin), TCSANOW, &nflags);
    #endif

    std::getline(std::cin, retval);
    
    #if WIN32
        SetConsoleMode(hStdin, mode);
    #else // !WIN32
        tcsetattr(fileno(stdin), TCSANOW, &oflags);
    #endif
}

std::mutex Debug::mutex;
Debug::Level Debug::level = Debug::Level::NONE;
high_resolution_clock::time_point Debug::start = high_resolution_clock::now();

/*****************************************************/
Debug::operator bool() const
{
    return static_cast<bool>(level);
}

/*****************************************************/
void Debug::Info(const std::string& str)
{
    if (level >= Level::INFO) Error(str);

    if (str.empty()) this->buffer.str(std::string());
}

/*****************************************************/
void Debug::Backend(const std::string& str)
{
    if (level >= Level::BACKEND) Error(str);

    if (str.empty()) this->buffer.str(std::string());
}

/*****************************************************/
void Debug::Error(const std::string& str)
{
    if (level < Level::ERRORS) return;

    const std::lock_guard<std::mutex> lock(mutex);

    std::ostream& out(level == Level::ERRORS ? std::cout : std::cerr);

    if (level >= Level::DETAILS)
    {
        duration<double> time = high_resolution_clock::now() - start;
        out << "time:" << time.count() << " ";

        out << "tid:" << std::this_thread::get_id() << " ";

        if (this->addr != nullptr) out << "obj:" << this->addr << " ";
    }

    out << this->prefix << ": ";

    if (str.empty())
    {
        out << this->buffer.str() << std::endl; 
        
        this->buffer.str(std::string()); // reset buffer
    }
    else out << str << std::endl;
}

/*****************************************************/
void Debug::DumpBytes(const void* ptr, size_t bytes, size_t width)
{
    this->buffer << "printing " << bytes << " bytes at " 
        << std::hex << ptr << std::endl;

    for (size_t i = 0; i < bytes; i++)
    {
        const uint8_t* byte = static_cast<const uint8_t*>(ptr)+i;
        
        if (i % width == 0) this->buffer << static_cast<const void*>(byte) << ": ";
        
        // need to cast to a 16-bit integer so it gets printed as a number not a character
        this->buffer << std::setw(2) << std::setfill('0') << static_cast<uint16_t>(*byte) << " ";

        if (i % width == width-1) this->buffer << std::endl;
    }

    this->buffer << std::endl;
}

} // namespace Andromeda
