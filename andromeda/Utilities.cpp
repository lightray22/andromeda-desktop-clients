#include <iostream>
#include <thread>

#include "Utilities.hpp"

std::vector<std::string> Utilities::explode(
    std::string str, const std::string& delim, 
    const int max, const size_t skip)
{
    size_t skipped = 0, start = 0, end;
    std::vector<std::string> retval;

    while ((end = str.find(delim, start)) != std::string::npos
            && (max < 0 || retval.size() < max - 1))
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
    return std::move(retval);
}

std::mutex Debug::mutex;

Debug::Level Debug::level = Debug::Level::DEBUG_NONE;

void Debug::Out(Debug::Level level, const std::string& str)
{
    if (level <= Debug::level) Debug::Print(str);
}

void Debug::Print(const std::string& str)
{
    // TODO log time also?

    const std::lock_guard<std::mutex> lock(Debug::mutex);

    std::cout << this->prefix << ":";
    
    if (level >= Debug::Level::DEBUG_DETAILS)
    {
        std::cout << " tid:" << std::this_thread::get_id();

        if (this->addr != nullptr) std::cout << " @" << this->addr;
    }

    std::cout << ": ";

    if (str.empty())
    {
        std::cout << this->buffer.str() << std::endl; 
        
        this->buffer.str(std::string()); // reset buffer
    }
    else std::cout << str << std::endl;
}

Debug& Debug::operator<<(const std::string& str)
{ 
    this->buffer << str; return *this; 
}