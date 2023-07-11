#include "catch2/catch_test_macros.hpp"
#include "trompeloeil.hpp"

#include "database/ObjectDatabase.hpp"
#include "database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

class MockSqliteDatabase : public SqliteDatabase
{
public:
    MAKE_MOCK2(query, size_t(const std::string&, const MixedParams&)); // not virtual, don't override
    MAKE_MOCK3(query, size_t(const std::string&, const MixedParams&, RowList&)); // not virtual, don't override
};

} // namespace Database
} // namespace Andromeda
