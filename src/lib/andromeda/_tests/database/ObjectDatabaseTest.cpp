#include "catch2/catch_test_macros.hpp"
#include "catch2/trompeloeil.hpp"

#include "testObjects.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

// TODO !! unit tests

/*****************************************************/
TEST_CASE("GetClassTable", "[ObjectDatabase]")
{
    REQUIRE(ObjectDatabase::GetClassTableName("Andromeda\\Database\\EasyObject") == "a2obj_database_easyobject");
}

} // namespace Database
} // namespace Andromeda
