
#include <cstring>
#include <sqlite3.h>

#include "MixedValue.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
MixedValue::MixedValue(sqlite3_value& value) noexcept : 
    mValue(sqlite3_value_dup(&value)) { }

/*****************************************************/
MixedValue::MixedValue(MixedValue&& old) noexcept : // move
    mValue(old.mValue)
{
    old.mValue = nullptr;
}

/*****************************************************/
MixedValue::~MixedValue()
{
    sqlite3_value_free(mValue);
}

/*****************************************************/
MixedValue::operator MixedValue::Bytes() const noexcept
{
    Bytes retval(static_cast<size_t>(sqlite3_value_bytes(mValue)));
    std::memcpy(retval.data(), sqlite3_value_blob(mValue), retval.size());
    return retval;
}

/*****************************************************/
MixedValue::operator std::string() const noexcept
{
    return reinterpret_cast<const char*>(sqlite3_value_text(mValue));
}

/*****************************************************/
MixedValue::operator int() const noexcept
{
    return sqlite3_value_int(mValue);
}

/*****************************************************/
MixedValue::operator int64_t() const noexcept
{
    return sqlite3_value_int64(mValue);
}

/*****************************************************/
MixedValue::operator double() const noexcept
{
    return sqlite3_value_double(mValue);
}

} // namespace Database
} // namespace Andromeda
