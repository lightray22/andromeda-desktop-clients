
#include "Debug.hpp"

#include <iomanip>
#include <iostream>
#include <thread>

using namespace std::chrono;

namespace Andromeda {

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
}

/*****************************************************/
void Debug::Backend(const std::string& str)
{
    if (sLevel >= Level::BACKEND) Error(str);
}

/*****************************************************/
void Debug::Error(const std::string& str)
{
    if (sLevel < Level::ERRORS) return;

    const std::lock_guard<decltype(sMutex)> lock(sMutex);

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
    const std::lock_guard<decltype(sMutex)> lock(sMutex);
    
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
