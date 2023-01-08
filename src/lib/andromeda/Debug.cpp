
#include "Debug.hpp"

#include <iomanip>
#include <thread>

using namespace std::chrono;

namespace Andromeda {

std::mutex Debug::sMutex;
Debug::Level Debug::sLevel { Debug::Level::ERRORS };
std::unordered_set<std::string> Debug::sPrefixes;
high_resolution_clock::time_point Debug::sStart { high_resolution_clock::now() };

/*****************************************************/
void Debug::Print(Debug::StreamFunc& func)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    if (!sPrefixes.empty() && sPrefixes.find(mPrefix) == sPrefixes.end()) return;

    if (sLevel >= Level::DETAILS)
    {
        duration<double> time { high_resolution_clock::now() - sStart };
        std::cerr << "time:" << time.count() << " ";

        std::cerr << "tid:" << std::this_thread::get_id() << " ";

        if (mAddr == nullptr) { std::cerr << "static "; }
        else { std::cerr << "obj:" << mAddr << " "; }
    }

    std::cerr << mPrefix << ": "; func(std::cerr); std::cerr << std::endl;
}

/*****************************************************/
Debug::StreamFunc Debug::DumpBytes(const void* ptr, uint64_t bytes, uint8_t width)
{
    return [&](std::ostream& str)
    {
        str << "printing " << bytes << " bytes at " 
            << std::hex << ptr << std::endl;

        for (decltype(bytes) i { 0 }; i < bytes; i++)
        {
            const uint8_t* byte { static_cast<const uint8_t*>(ptr)+i };
            
            if (i % width == 0) str << static_cast<const void*>(byte) << ": ";
            
            // need to cast to a 16-bit integer so it gets printed as a number not a character
            str << std::setw(2) << std::setfill('0') << static_cast<uint16_t>(*byte) << " ";

            if ((i % width) + 1 == width) str << std::endl;
        }

        str << std::endl;
    };
}

} // namespace Andromeda
