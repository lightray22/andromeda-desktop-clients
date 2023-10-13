
#include "Debug.hpp"
#include "StringUtil.hpp"

#include <algorithm>
#include <iomanip>
#include <thread>

using std::chrono::steady_clock;
using std::chrono::duration;

namespace Andromeda {

std::mutex Debug::sMutex;
steady_clock::time_point Debug::sStart { steady_clock::now() };
std::vector<Debug::Context> Debug::sContexts;
Debug::Level Debug::sMaxLevel { Debug::Level::ERRORS };
std::list<std::ofstream> Debug::sFileStreams;

/*****************************************************/
Debug::Level Debug::GetMaxLevel()
{
    const decltype(sContexts)::const_iterator it { 
        std::max_element(sContexts.cbegin(), sContexts.cend(),
            [](const Context& ctx1, const Context& ctx2){ return ctx1.level < ctx2.level; }) };
    return (it != sContexts.cend()) ? it->level : Level::ERRORS;
}

/*****************************************************/
void Debug::SetLevel(Level level) // set all
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    for (Context& ctx : sContexts)
        ctx.level = level;
    sMaxLevel = level;
}

/*****************************************************/
void Debug::SetLevel(Level level, const std::ostream& stream)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    for (Context& ctx : sContexts)
        if (&stream == ctx.stream)
            ctx.level = level;
    sMaxLevel = GetMaxLevel();
}

/*****************************************************/
decltype(Debug::Context::filters) Debug::GetFilterSet(const std::string& filters)
{
    decltype(Context::filters) filterSet;
    for (const std::string& prefix : StringUtil::explode(filters,","))
        filterSet.emplace(StringUtil::trim(prefix));
    return filterSet;
}

/*****************************************************/
void Debug::SetFilters(const std::string& filters) // set all
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    const decltype(Context::filters) filterSet { GetFilterSet(filters) };
    for (Context& ctx : sContexts)
        ctx.filters = filterSet; // copy
}

/*****************************************************/
void Debug::SetFilters(const std::string& filters, const std::ostream& stream)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    const decltype(Context::filters) filterSet { GetFilterSet(filters) };
    for (Context& ctx : sContexts)
        if (&stream == ctx.stream)
            ctx.filters = filterSet; // copy
}

/*****************************************************/
void Debug::AddStream(std::ostream& stream)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    sContexts.emplace_back(stream); // default level, filters
    sMaxLevel = GetMaxLevel();
}

/*****************************************************/
void Debug::RemoveStream(std::ostream& stream)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    const decltype(sContexts)::const_iterator it {
        std::find_if(sContexts.cbegin(), sContexts.cend(),
            [&](const Context& ctx){ return &stream == ctx.stream; }) };
    if (it != sContexts.cend()) sContexts.erase(it);
    sMaxLevel = GetMaxLevel();
}

/*****************************************************/
void Debug::AddLogFile(const std::string& path)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    sFileStreams.emplace_back(path, std::ofstream::out);
    std::ostream& stream { sFileStreams.back() };

    // copy config from the first existing stream, else use defaults
    if (!sContexts.empty())
    {
        Context context { sContexts.front() }; // copy
        context.stream = &stream;
        sContexts.emplace_back(context);
    }
    else sContexts.emplace_back(stream); // default level, filters
    sMaxLevel = GetMaxLevel();
}

/*****************************************************/
void Debug::Print(const Debug::StreamFunc& strfunc, Level level)
{
    const std::lock_guard<decltype(sMutex)> lock(sMutex);

    for (const Context& ctx : sContexts)
    {
        if (level > ctx.level) continue;
        if (level > Level::ERRORS && (!ctx.filters.empty() && ctx.filters.find(mPrefix) == ctx.filters.cend()))
            continue; // filtered out, do nothing

        std::ostream& stream { *ctx.stream };

        if (ctx.level >= Level::DETAILS)
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
