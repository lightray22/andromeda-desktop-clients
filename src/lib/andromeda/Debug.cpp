
#include "Debug.hpp"

#include <chrono>
#include <iomanip>
#include <mutex>
#include <thread>

using std::chrono::steady_clock;
using std::chrono::duration;

namespace Andromeda {

namespace { // anonymous
/** Global output lock */
std::mutex sMutex;
/** timestamp when the program started */
steady_clock::time_point sStart { steady_clock::now() };
} // namespace

Debug::Level Debug::sLevel { Debug::Level::ERRORS };
std::unordered_set<std::string> Debug::sPrefixes;

std::list<std::ostream*> Debug::sStreams;
std::list<std::ofstream> Debug::sFileStreams;

/*****************************************************/
void Debug::AddStream(const std::string& path)
{ 
    sFileStreams.emplace_back(path, std::ofstream::out);
    sStreams.emplace_back(&sFileStreams.back());
}

/*****************************************************/
void Debug::PrintIf(const Debug::StreamFunc& strfunc)
{
    if (sPrefixes.empty() || sPrefixes.find(mPrefix) != sPrefixes.end())
    {
        Print(strfunc);
    }
}

/*****************************************************/
void Debug::Print(const Debug::StreamFunc& strfunc)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    for (std::ostream* streamPtr : sStreams)
    {
        std::ostream& stream { *streamPtr };

        if (sLevel >= Level::DETAILS)
        {
            stream << "tid:" << std::this_thread::get_id() << " ";

            const duration<double> time { steady_clock::now() - sStart };
            stream << "time:" << time.count() << " ";

            if (mAddr == nullptr) { stream << "static "; }
            else { stream << "obj:" << mAddr << " "; }
        }

        stream << mPrefix << ": "; strfunc(stream); stream << std::endl;
    }
}

/*****************************************************/
Debug::StreamFunc Debug::DumpBytes(const void* ptr, size_t bytes, size_t width)
{
    // copy variables into std::function (scope)
    return [ptr,bytes,width](std::ostream& str)
    {
        const std::ios_base::fmtflags sflags { str.flags() };

        str << "printing " << bytes << " bytes at " 
            << std::hex << ptr << std::endl;

        for (decltype(bytes) i { 0 }; i < bytes; i++)
        {
            const uint8_t* byte { static_cast<const uint8_t*>(ptr)+i };
            
            if (i % width == 0) str << static_cast<const void*>(byte) << ": ";
            
            // need to cast to a 16-bit integer so it gets printed as a number not a character
            str << std::setw(2) << std::setfill('0') << static_cast<uint16_t>(*byte) << " ";

            if (i+1 < bytes && (i % width) + 1 == width) str << std::endl;
        }

        str.flags(sflags); // restore flags
    };
}

} // namespace Andromeda
