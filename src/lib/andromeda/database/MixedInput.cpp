
#include <sqlite3.h>

#include "MixedInput.hpp"

namespace Andromeda {
namespace Database {

// makes the compiler print the invalid type if used
template<class> inline constexpr bool always_false_v = false;

/*****************************************************/
int MixedInput::bind(sqlite3_stmt& stmt, int idx) const
{
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
    }, mValue);
}

/*****************************************************/
MixedParams::MixedParams(const std::initializer_list<MixedInputPair>& list) noexcept
{
    // this function seems like it does nothing more than the default, but...
    // it is necessary to tell the compiler that we're expecting MixedInputPairs
    // so that the typeless {{":d0",5},{":d1","test"}} syntax works

    for (const MixedInputPair& pair : list)
        emplace(pair.first, pair.second);
}

} // namespace Database
} // namespace Andromeda
