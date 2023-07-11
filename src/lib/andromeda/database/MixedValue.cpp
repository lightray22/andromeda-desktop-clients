
#include <sqlite3.h>

#include "MixedValue.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
MixedValue::MixedValue(const sqlite3_value& value) noexcept : 
    mSqlValue(sqlite3_value_dup(&value)) { }

/*****************************************************/
MixedValue::MixedValue(const MixedValue& src) noexcept : // copy
    mVariant(src.mVariant)
{
    if (src.mSqlValue != nullptr)
        mSqlValue = sqlite3_value_dup(src.mSqlValue);
}

/*****************************************************/
MixedValue::MixedValue(MixedValue&& old) noexcept : // move
    mSqlValue(old.mSqlValue),
    mVariant(old.mVariant)
{
    old.mSqlValue = nullptr; // don't free
}

/*****************************************************/
MixedValue::~MixedValue()
{
    sqlite3_value_free(mSqlValue);
}

// makes the compiler print the invalid type if used
template<class> inline constexpr bool always_false_v = false;

/*****************************************************/
int MixedValue::bind(sqlite3_stmt& stmt, int idx) const
{
    if (mSqlValue != nullptr) 
        return sqlite3_bind_value(&stmt, idx, mSqlValue);

    return std::visit([&](auto&& val)->int
    {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>)
        {
            return sqlite3_bind_null(&stmt, idx);
        }
        else if constexpr (std::is_same_v<T, const std::string*>)
        {
            return sqlite3_bind_blob(&stmt, idx, val->data(), 
                static_cast<int>(val->size()), nullptr);
        }
        else if constexpr (std::is_same_v<T, const char*>)
        {
            return sqlite3_bind_text(&stmt, idx, val, -1, nullptr);
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            return sqlite3_bind_int(&stmt, idx, val);
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            return sqlite3_bind_int64(&stmt, idx, val);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return sqlite3_bind_double(&stmt, idx, val);
        }
        else static_assert(always_false_v<T>, "missing mixed-type");
    }, mVariant);
}

/*****************************************************/
bool MixedValue::is_null() const
{
    if (mSqlValue != nullptr)
        return sqlite3_value_bytes(mSqlValue) == 0;
    else return std::holds_alternative<std::nullptr_t>(mVariant);
}

/*****************************************************/
void MixedValue::get_to(std::string& out) const
{
    if (mSqlValue != nullptr) out = std::string( // new copy
        static_cast<const char*>(sqlite3_value_blob(mSqlValue)),
        static_cast<size_t>(sqlite3_value_bytes(mSqlValue)));
    else out = *std::get<const std::string*>(mVariant);
}

/*****************************************************/
void MixedValue::get_to(const char*& out) const
{
    if (mSqlValue != nullptr) // reinterpret_cast, sqlite gives unsigned char
        out = reinterpret_cast<const char*>(sqlite3_value_text(mSqlValue));
    else out = std::get<const char*>(mVariant);
}

/*****************************************************/
void MixedValue::get_to(int& out) const
{
    if (mSqlValue != nullptr) 
        out = sqlite3_value_int(mSqlValue);
    else out = std::get<int>(mVariant);
}

/*****************************************************/
void MixedValue::get_to(int64_t& out) const
{
    if (mSqlValue != nullptr) 
        out = sqlite3_value_int64(mSqlValue);
    else out = std::get<int64_t>(mVariant);
}

/*****************************************************/
void MixedValue::get_to(double& out) const
{
    if (mSqlValue != nullptr) 
        out = sqlite3_value_double(mSqlValue);
    else out = std::get<double>(mVariant);
}

/*****************************************************/
MixedParams::MixedParams(const std::initializer_list<MixedValuePair>& list) noexcept
{
    // this function seems like it does nothing more than the default, but...
    // it is necessary to tell the compiler that we're expecting MixedValuePairs
    // so that the typeless {{":d0",5},{":d1","test"}} syntax works

    for (const MixedValuePair& pair : list)
        emplace(pair.first, pair.second);
}

} // namespace Database
} // namespace Andromeda
