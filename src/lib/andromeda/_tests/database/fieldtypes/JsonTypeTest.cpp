
#include "nlohmann/json.hpp"
#include "catch2/catch_test_macros.hpp"

#include "../testObjects.hpp"
#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/fieldtypes/JsonType.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

#define GET_MOCK_OBJECTS() \
    MockSqliteDatabase sqldb; \
    ObjectDatabase objdb(sqldb); \
    EasyObject parent(objdb,{});

/*****************************************************/
TEST_CASE("BasicJson", "[JsonType]")
{
    GET_MOCK_OBJECTS();
    
    JsonType field("myjson",parent);
    REQUIRE(field.is_null() == true);
    REQUIRE(field.GetDBValue() == nullptr);
    REQUIRE(field.TryGetJson() == nullptr);

    nlohmann::json testJ1 {"myttt",45};
    field.InitDBValue(MixedValue(testJ1.dump()));
    REQUIRE(field.is_null() == false);
    REQUIRE(field.isModified() == false);
    REQUIRE(*field.TryGetJson() == testJ1);

    nlohmann::json testJ2 {"mytest",58};
    field.SetJson(testJ2);
    REQUIRE(field.isModified() == true);
    REQUIRE(field.GetDBValue() == testJ2.dump());
    REQUIRE(*field.TryGetJson() == testJ2);

    field.SetJson(nullptr);
    REQUIRE(field.is_null() == true);
}

/*****************************************************/
TEST_CASE("DefaultJson", "[JsonType]")
{
    GET_MOCK_OBJECTS();
    
    nlohmann::json testJ {"mytest",58};
    const JsonType field("myjson",parent,testJ);

    REQUIRE(field.isModified() == true);
    REQUIRE(field.is_null() == false);
    REQUIRE(*field.TryGetJson() == testJ);
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda
