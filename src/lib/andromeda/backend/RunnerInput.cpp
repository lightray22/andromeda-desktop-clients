
#include "RunnerInput.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
RunnerInput_StreamIn::WriteFunc RunnerInput_StreamIn::FromStream(std::istream& data)
{
    // curoff is copied to within the std::function and maintains state between successive calls
    size_t curoff = 0; return [&data,curoff](const size_t offset, char* const buf, const size_t length, size_t& read) mutable ->bool
    {
        if (offset != curoff)
        {
            data.seekg(static_cast<std::streamsize>(offset));
            if (!data.good()) throw StreamSeekException();
            curoff = offset;
        }

        data.read(buf, static_cast<std::streamsize>(length));
        read = static_cast<std::size_t>(data.gcount());
        curoff += read;

        if (data.bad() || (data.fail() && !data.eof()))
            throw StreamFailException();

        return !data.eof();
    };
}

/*****************************************************/
RunnerInput_StreamOut::ReadFunc RunnerInput_StreamOut::ToStream(std::ostream& data)
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
