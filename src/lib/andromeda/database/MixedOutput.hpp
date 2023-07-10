#ifndef LIBA2_MIXEDOUTPUT_H_
#define LIBA2_MIXEDOUTPUT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

struct sqlite3_value;

namespace Andromeda {
namespace Database {

/** Holds a mixed-type value retrieved from the database */
class MixedOutput
{
public:

    /** Initialize (copy) from an sqlite3_value */
    explicit MixedOutput(sqlite3_value& value) noexcept;
    MixedOutput(MixedOutput&& old) noexcept; // move

    virtual ~MixedOutput();

    MixedOutput(const MixedOutput&) = delete; // no copy
    MixedOutput& operator=(const MixedOutput&) = delete;
    MixedOutput& operator=(MixedOutput&&) = delete;

    /** Returns true if the value is SQL NULL */
    [[nodiscard]] bool is_null() const;

    explicit operator std::string() const noexcept;
    explicit operator const char*() const noexcept;
    explicit operator int() const noexcept;
    explicit operator int64_t() const noexcept;
    explicit operator double() const noexcept;

    /** Returns the value as the desired type */
    template <typename T>
    [[nodiscard]] inline T get() const noexcept
    {
        return static_cast<T>(*this);
    }

    /** Stores the value in the desired out variable */
    template <typename T>
    inline void get_to(T& out) const noexcept
    {
        out = static_cast<T>(*this);
    }

    /** Compare to any type by first casting to it */
    template<typename T>
    inline bool operator==(const T& cmp) const noexcept
    {
        return static_cast<T>(*this) == cmp;
    }

    /** Can't cast to std::nullptr_t so use is_null() */
    inline bool operator==(std::nullptr_t) const noexcept
    {
        return is_null();
    }

    /** Allow comparing correctly to const char* using strcmp */
    inline bool operator==(const char* str) const noexcept
    {
        return !std::strcmp(get<const char*>(), str);
    }

private:

    sqlite3_value* mValue;
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_MIXEDOUTPUT_H_
