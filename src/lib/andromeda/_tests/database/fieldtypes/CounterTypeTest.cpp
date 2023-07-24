
#include "catch2/catch_test_macros.hpp"

#include "../testObjects.hpp"
#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/fieldtypes/CounterType.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

#define GET_MOCK_OBJECTS() \
    MockSqliteDatabase sqldb; \
    ObjectDatabase objdb(sqldb); \
    EasyObject parent(objdb,{});

/*****************************************************/
TEST_CASE("Counter", "[FieldTypes]")
{
    GET_MOCK_OBJECTS();
    
    CounterType field("mycounter",parent);
    REQUIRE(field.GetValue() == 0); // default
    REQUIRE(field.isModified() == false);
    REQUIRE(field.UseDBIncrement() == true);

    field.InitDBValue(MixedValue(5));
    REQUIRE(field.GetValue() == 5);

    REQUIRE(field.DeltaValue(10) == true);
    REQUIRE(field.GetValue() == 15);
    REQUIRE(field.GetDBValue() == 10); // delta
    REQUIRE(field.isModified());

    // operators
    field += 7;
    field -= 3;
    REQUIRE(field == 19);
    REQUIRE(field != 20);
    REQUIRE(static_cast<int>(field) == 19);
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda
