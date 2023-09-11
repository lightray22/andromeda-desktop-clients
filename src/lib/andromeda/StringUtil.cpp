
#include <algorithm>
#include <array>
#include <locale>
#include <random>

#include "StringUtil.hpp"

namespace Andromeda {

/*****************************************************/
std::string StringUtil::Random(const size_t size)
{
    static constexpr std::array<char,37> chars { "0123456789abcdefghijkmnopqrstuvwxyz_" }; // 36+NUL
    std::default_random_engine rng(std::random_device{}());

    // set the max to chars-2 as chars has a NUL term, and dist includes max
    std::uniform_int_distribution<> dist(0, chars.size()-2);

    std::string retval; retval.resize(size);
    for (size_t i { 0 }; i < size; ++i)
        retval[i] = chars[static_cast<size_t>(dist(rng))];
    return retval;
}

/*****************************************************/
StringUtil::StringList StringUtil::explode(
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
StringUtil::StringPair StringUtil::split(
    const std::string& str, const std::string& delim, 
    const size_t skip, const bool reverse)
{
    StringUtil::StringList list { explode(str, delim, skip, reverse, 2) };

    while (list.size() < 2)
    {
        if (!reverse) list.emplace_back("");
        else list.insert(list.begin(),"");
    }

    return StringUtil::StringPair { list[0], list[1] };
}

/*****************************************************/
StringUtil::StringPair StringUtil::splitPath(const std::string& stri)
{
    std::string str { stri }; // copy
    while (!str.empty() && str.back() == '/')
        str.pop_back(); // remove trailing /
    
    return split(str,"/",0,true);
}

/*****************************************************/
bool StringUtil::startsWith(const std::string& str, const std::string& start)
{
    if (start.size() > str.size()) return false;

    return std::equal(start.begin(), start.end(), str.begin());
}

/*****************************************************/
bool StringUtil::endsWith(const std::string& str, const std::string& end)
{
    if (end.size() > str.size()) return false;

    return std::equal(end.rbegin(), end.rend(), str.rbegin());
}

/*****************************************************/
void StringUtil::trim_void(std::string& str)
{
    const size_t size { str.size() };

    size_t start = 0; while (start < size && std::isspace(str[start])) ++start;
    size_t end = size; while (end > 0 && std::isspace(str[end-1])) --end;

    str.erase(end); str.erase(0, start);
}

/*****************************************************/
std::string StringUtil::trim(const std::string& str)
{
    std::string retval { str }; // copy
    trim_void(retval); return retval;
}

/*****************************************************/
std::string StringUtil::tolower(const std::string& str)
{
    std::string ret(str);
    std::transform(ret.begin(), ret.end(), ret.begin(),
        [](char c)->char{ return std::tolower(c, std::locale()); });
    return ret;
}

/*****************************************************/
void StringUtil::replaceAll_void(std::string& str, const std::string& from, const std::string& repl)
{
    if (from.empty()) return; // invalid

    for (size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += repl.size())
        str.replace(pos, from.size(), repl);
}

/*****************************************************/
std::string StringUtil::replaceAll(const std::string& str, const std::string& from, const std::string& repl)
{
    std::string retval(str); // copy
    replaceAll_void(retval, from, repl); return retval;
}

/*****************************************************/
std::string StringUtil::escapeAll(const std::string& str, const std::vector<char>& delims, const char escape)
{
    std::string retval(str); // copy

    const std::string escapeStr(1,escape);
    replaceAll_void(retval, escapeStr, std::string(2,escape));

    for (const char& delim : delims)
    {
        const std::string delimStr(1,delim);
        replaceAll_void(retval, delimStr, escapeStr+delimStr);
    }
    return retval;
}

/*****************************************************/
bool StringUtil::stringToBool(const std::string& stri)
{
    const std::string str { trim(stri) };
    return (!str.empty() && str != "0" && str != "false" && str != "off" && str != "no");
}

static constexpr size_t bytesMul { 1024 };

/*****************************************************/
uint64_t StringUtil::stringToBytes(const std::string& stri)
{
    std::string str { trim(stri) };
    if (str.empty()) return 0;

    const char unit { str.at(str.size()-1) };
    
    if (unit < '0' || unit > '9')
    {
        str.pop_back(); trim_void(str);
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
std::string StringUtil::bytesToString(uint64_t bytes)
{
    size_t unit { 0 };
    static constexpr std::array<const char*,6> units { "", "K", "M", "G", "T", "P" };
    while (bytes >= bytesMul && !(bytes % bytesMul) && unit < units.size()-1) {
        ++unit; bytes /= bytesMul;
    }
    return std::to_string(bytes)+units[unit];
}

} // namespace Andromeda
