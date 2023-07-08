#ifndef LIBA2_OBJECTDATABASE_H_
#define LIBA2_OBJECTDATABASE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct sqlite3_value;

namespace Andromeda {
namespace Database {

class MixedValue
{
public:

    explicit MixedValue(sqlite3_value& value) noexcept;
    MixedValue(MixedValue&& old) noexcept; // move

    virtual ~MixedValue();

    MixedValue(const MixedValue&) = delete; // no copy
    MixedValue& operator=(const MixedValue&) = delete;
    MixedValue& operator=(MixedValue&&) = delete;

    [[nodiscard]] bool is_null() const;

    explicit operator std::string() const noexcept;
    explicit operator const char*() const noexcept;
    explicit operator int() const noexcept;
    explicit operator int64_t() const noexcept;
    explicit operator double() const noexcept;

private:

    sqlite3_value* mValue { nullptr };
};

} // namespace Database
} // namespace Andromeda

#endif // LIBA2_OBJECTDATABASE_H_
