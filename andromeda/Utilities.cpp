#include <iostream>
#include <thread>
#include <chrono>
#include <termios.h>
#include <fstream>

#include "Utilities.hpp"

using namespace std::chrono;

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
bool Utilities::stringToBool(const std::string& str)
{
    return (str != "0" && str != "false" && str != "off" && str != "no");
}

/*****************************************************/
bool Utilities::parseArgs(int argc, char** argv, 
    Utilities::Flags& flags, Utilities::Options& options)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-') return false;

        const char* flag = argv[i]+1;
        if (argc-1 > i && argv[i+1][0] != '-')
            options.emplace(flag, argv[++i]);
        else flags.push_back(flag);
    }

    return true;
}

/*****************************************************/
void Utilities::parseFile(const std::filesystem::path& path, Flags& flags, Options& options)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    while (file.good())
    {
        std::string line; std::getline(file,line);

        if (!line.size() || line.at(0) == '#') continue;

        StringPair pair(split(line, "="));

        pair.first = "-"+pair.first;

        if (pair.second.size()) 
             options.emplace(pair);
        else flags.push_back(pair.first);
    }
}

/*****************************************************/
void Utilities::SilentReadConsole(std::string& retval)
{
    struct termios oflags, nflags;
    
    tcgetattr(fileno(stdin), &oflags);

    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    tcsetattr(fileno(stdin), TCSANOW, &nflags);

    std::getline(std::cin, retval);

    tcsetattr(fileno(stdin), TCSANOW, &oflags);
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
