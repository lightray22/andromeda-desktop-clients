
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
    mVariant(std::move(old.mVariant))
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
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return sqlite3_bind_blob(&stmt, idx, val.data(), 
                static_cast<int>(val.size()), nullptr);
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
std::string MixedValue::ToString() const
{
    if (mSqlValue != nullptr)
        return get<std::string>();

    return std::visit([&](auto&& val)->std::string
    {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) return "NULL";
        else if constexpr (std::is_same_v<T, std::string>) return val;
        else if constexpr (std::is_same_v<T, const char*>) return std::string(val);
        else return std::to_string(val); // int, int64_t, double
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

    else if (std::holds_alternative<const char*>(mVariant)) // allow conversion
        out = std::get<const char*>(mVariant);

    else out = std::get<std::string>(mVariant);
}

/*****************************************************/
void MixedValue::get_to(const char*& out) const
{
    if (mSqlValue != nullptr) // reinterpret_cast, sqlite gives unsigned char
        out = reinterpret_cast<const char*>(sqlite3_value_text(mSqlValue));

    else if (std::holds_alternative<std::string>(mVariant)) // allow conversion
        out = std::get<std::string>(mVariant).c_str();

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
bool MixedValue::operator==(const MixedValue& rhs) const
{
    if (this == &rhs) return true;
    
    if (mSqlValue != nullptr && rhs.mSqlValue != nullptr)
    {
        int bytes { 0 };
        return sqlite3_value_type(mSqlValue) == sqlite3_value_type(rhs.mSqlValue)
            && (bytes = sqlite3_value_bytes(mSqlValue)) == sqlite3_value_bytes(rhs.mSqlValue)
            && !memcmp(sqlite3_value_blob(mSqlValue), sqlite3_value_blob(rhs.mSqlValue), static_cast<size_t>(bytes));
    }
    
    return std::visit([&](auto&& val)->bool
    {
        // const char* and std::string* are pointers, we want to compare content not just pointer equality
        // so these require special handling.  Also allow comparing string vs. char* or vice versa.

        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::string>)
        {
            std::string const* rh1 = std::get_if<std::string>(&rhs.mVariant);
            if (rh1 != nullptr) return val == *rh1;

            const char* const* rh2 = std::get_if<const char*>(&rhs.mVariant);
            if (rh2 != nullptr) return val == *rh2;

            return false; // wrong type
        }
        else if constexpr (std::is_same_v<T, const char*>)
        {
            std::string const* rh1 = std::get_if<std::string>(&rhs.mVariant);
            if (rh1 != nullptr) return !std::strcmp(val, (*rh1).c_str());

            const char* const* rh2 = std::get_if<const char*>(&rhs.mVariant);
            if (rh2 != nullptr) return !std::strcmp(val, *rh2);

            return false; // wrong type
        }
        else return mVariant == rhs.mVariant;
    }, mVariant);
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
