#ifndef LIBA2_OBJECTDATABASE_H_
#define LIBA2_OBJECTDATABASE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct sqlite3_value;

namespace Andromeda {
namespace Database {

class MixedOutput
{
public:

    explicit MixedOutput(sqlite3_value& value) noexcept;
    MixedOutput(MixedOutput&& old) noexcept; // move

    virtual ~MixedOutput();

    MixedOutput(const MixedOutput&) = delete; // no copy
    MixedOutput& operator=(const MixedOutput&) = delete;
    MixedOutput& operator=(MixedOutput&&) = delete;

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
