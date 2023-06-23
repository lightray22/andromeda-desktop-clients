
#include "Debug.hpp"

#include <chrono>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <thread>

using std::chrono::steady_clock;
using std::chrono::duration;

namespace Andromeda {

/** Global output lock */
static std::mutex sMutex;
/** timestamp when the program started */
static steady_clock::time_point sStart { steady_clock::now() };

Debug::Level Debug::sLevel { Debug::Level::ERRORS };
std::unordered_set<std::string> Debug::sPrefixes;

static constexpr std::ostream& sOutstr { std::cerr };
//static constexpr std::ofstream sOutstr("/tmp/debug.log", std::ofstream::out);

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

    if (sLevel >= Level::DETAILS)
    {
        sOutstr << "tid:" << std::this_thread::get_id() << " ";

        const duration<double> time { steady_clock::now() - sStart };
        sOutstr << "time:" << time.count() << " ";

        if (mAddr == nullptr) { sOutstr << "static "; }
        else { sOutstr << "obj:" << mAddr << " "; }
    }

    sOutstr << mPrefix << ": "; strfunc(sOutstr); sOutstr << std::endl;
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

            if ((i % width) + 1 == width) str << std::endl;
        }

        str << std::endl; str.flags(sflags); // restore flags
    };
}

} // namespace Andromeda
