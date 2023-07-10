
#include <sqlite3.h>

#include "MixedOutput.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
MixedOutput::MixedOutput(sqlite3_value& value) noexcept : 
    mValue(sqlite3_value_dup(&value)) { }

/*****************************************************/
MixedOutput::MixedOutput(MixedOutput&& old) noexcept : // move
    mValue(old.mValue)
{
    old.mValue = nullptr;
}

/*****************************************************/
MixedOutput::~MixedOutput()
{
    sqlite3_value_free(mValue);
}

/*****************************************************/
bool MixedOutput::is_null() const
{
    return sqlite3_value_bytes(mValue) == 0;
}

/*****************************************************/
MixedOutput::operator std::string() const noexcept
{
    return std::string(
        static_cast<const char*>(sqlite3_value_blob(mValue)),
        static_cast<size_t>(sqlite3_value_bytes(mValue)));
}

/*****************************************************/
MixedOutput::operator const char*() const noexcept
{
    // need reinterpret_cast, sqlite gives unsigned char
    return reinterpret_cast<const char*>(sqlite3_value_text(mValue));
}

/*****************************************************/
MixedOutput::operator int() const noexcept
{
    return sqlite3_value_int(mValue);
}

/*****************************************************/
MixedOutput::operator int64_t() const noexcept
{
    return sqlite3_value_int64(mValue);
}

/*****************************************************/
MixedOutput::operator double() const noexcept
{
    return sqlite3_value_double(mValue);
}

} // namespace Database
} // namespace Andromeda
