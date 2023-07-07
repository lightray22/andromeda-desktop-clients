#include "catch2/catch_test_macros.hpp"

#include "andromeda/TempPath.hpp"
#include "andromeda/database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

/*****************************************************/
TEST_CASE("Construct", "[SqliteDatabase]")
{
    const TempPath tmppath("test_sqlite.s3db"); 
    const SqliteDatabase database(tmppath.Get());
}

} // namespace Database
} // namespace Andromeda
