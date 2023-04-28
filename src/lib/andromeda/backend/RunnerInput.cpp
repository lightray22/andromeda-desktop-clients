
#include "RunnerInput.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
size_t RunnerInput_StreamIn::StreamSize(const WriteFunc& func)
{
    size_t retval { 0 }; 
    bool moreData { true }; while (moreData)
    {
        size_t sread { 0 };
        char buf[1024*1024];
        moreData = func(retval, buf, sizeof(buf), sread);
        retval += sread;
    }
    return retval;
}

/*****************************************************/
WriteFunc RunnerInput_StreamIn::FromString(const std::string& data)
{
    return [&](const size_t soffset, char* const buf, const size_t buflen, size_t& sread)->bool
    {
        if (soffset >= data.size()) return false;

        const char* sdata { data.data()+soffset };
        sread = std::min(data.size()-soffset, buflen);
        
        std::copy(sdata, sdata+sread, buf); 
        return soffset+sread < data.size();
    };
}

/*****************************************************/
WriteFunc RunnerInput_StreamIn::FromStream(std::istream& data)
{
    // curoff is copied to within the std::function and maintains state between successive calls
    size_t curoff = 0; return [&data,curoff](const size_t offset, char* const buf, const size_t buflen, size_t& read) mutable ->bool
    {
        if (offset != curoff)
        {
            data.seekg(static_cast<std::streamsize>(offset));
            if (!data.good()) throw StreamSeekException();
            curoff = offset;
        }

        data.read(buf, static_cast<std::streamsize>(buflen));
        read = static_cast<std::size_t>(data.gcount());
        curoff += read;

        if (data.bad() || (data.fail() && !data.eof()))
            throw StreamFailException();

        return !data.eof();
    };
}

/*****************************************************/
ReadFunc RunnerInput_StreamOut::ToStream(std::ostream& data)
{
    // curoff is copied to within the std::function and maintains state between successive calls
    size_t curoff = 0; return [&data,curoff](const size_t offset, const char* buf, const size_t length) mutable ->void 
    {
        if (offset != curoff)
        {
            data.seekp(static_cast<std::streamsize>(offset));
            if (!data.good()) throw StreamSeekException();
            curoff = offset;
        }

        data.write(buf, static_cast<std::streamsize>(length));
        curoff += length;

        if (data.fail()) throw StreamFailException();
    };
}

} // namespace Backend
} // namespace Andromeda
