
#include "catch2/catch_test_macros.hpp"
#include "catch2/trompeloeil.hpp"

#include "../testObjects.hpp"
#include "andromeda/database/BaseObject.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/QueryBuilder.hpp"
#include "andromeda/database/fieldtypes/ObjectRef.hpp"

namespace Andromeda {
namespace Database {
namespace FieldTypes {

using trompeloeil::_;
using Row = SqliteDatabase::Row;

#define GET_MOCK_OBJECTS() \
    MockSqliteDatabase sqldb; \
    ObjectDatabase objdb(sqldb); \
    EasyObject parent(objdb,{});

/*****************************************************/
TEST_CASE("NullObject", "[ObjectRef]")
{
    GET_MOCK_OBJECTS();

    NullObjectRef<EasyObject2> field("myobj",parent);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject2 WHERE id = :d0")
        .WITH(_2 == MixedParams{{":d0","abcd"}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abcd"}})).RETURN(0UL);

    QueryBuilder q; q.Where(q.Equals("id","abcd"));
    EasyObject2* testObj { objdb.TryLoadUniqueByQuery<EasyObject2>(q) };
    REQUIRE(testObj != nullptr);
    // simulate a real load so the obj exists in the objdb for checking ==

    REQUIRE(field.is_null() == true);
    REQUIRE(field.isModified() == false);
    REQUIRE(field.SetObject(nullptr) == false);
    REQUIRE(field.isModified() == false);
    REQUIRE(field.TryGetObject() == nullptr);
    REQUIRE(field.GetDBValue() == nullptr);

    field.InitDBValue(MixedValue("abcd"));

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject2 WHERE id = :d0")
        .WITH(_2 == MixedParams{{":d0","abcd"}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abcd"}})).RETURN(0UL);

    REQUIRE(field.is_null() == false);
    REQUIRE(*field.TryGetObject() == *testObj);
    REQUIRE(*field.TryGetObject() == *testObj); // only query once!
    REQUIRE(field.GetDBValue() == "abcd");
    REQUIRE(field.SetObject(*testObj) == false);

    REQUIRE(field.SetObject(nullptr) == true);
    REQUIRE(field.SetObject(*testObj) == true);
    REQUIRE(field.isModified() == true);

    REQUIRE(field.is_null() == false);
    REQUIRE(*field.TryGetObject() == *testObj); // no query!
}

/*****************************************************/
TEST_CASE("NonNullObjectInit", "[ObjectRef]")
{
    GET_MOCK_OBJECTS();

    ObjectRef<EasyObject2> field("myobj",parent);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject2 WHERE id = :d0")
        .WITH(_2 == MixedParams{{":d0","abcd"}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abcd"}})).RETURN(0UL);

    QueryBuilder q; q.Where(q.Equals("id","abcd"));
    EasyObject2* testObj { objdb.TryLoadUniqueByQuery<EasyObject2>(q) };
    REQUIRE(testObj != nullptr);
    // simulate a real load so the obj exists in the objdb for checking ==

    REQUIRE(field.isInitialized() == false);
    REQUIRE(field.isModified() == false);
    REQUIRE_THROWS_AS(field.GetObject(), BaseField::UninitializedException);
    REQUIRE_THROWS_AS(field.GetDBValue(), BaseField::UninitializedException);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject2 WHERE id = :d0")
        .WITH(_2 == MixedParams{{":d0","abcd"}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abcd"}})).RETURN(0UL);

    field.InitDBValue(MixedValue("abcd"));
    REQUIRE(field.isInitialized() == true);
    REQUIRE(field.isModified() == false);
    REQUIRE(field.GetObject() == *testObj);
    REQUIRE(field.GetObject() == *testObj); // only query once!
    REQUIRE(field.GetDBValue() == "abcd");
    REQUIRE(field.SetObject(*testObj) == false);
}

/*****************************************************/
TEST_CASE("NonNullObjectSet", "[ObjectRef]")
{
    GET_MOCK_OBJECTS();

    ObjectRef<EasyObject2> field("myobj",parent);
    EasyObject2 testObj(objdb,MixedParams{{"id","abcd"}});

    REQUIRE(field.isInitialized() == false);
    REQUIRE(field.isModified() == false);
    REQUIRE_THROWS_AS(field.GetObject(), BaseField::UninitializedException);
    REQUIRE_THROWS_AS(field.GetDBValue(), BaseField::UninitializedException);

    field.SetObject(testObj);
    REQUIRE(field.isInitialized() == true);
    REQUIRE(field.isModified() == true);
    REQUIRE(field.GetObject() == testObj); // no query!
    REQUIRE(field.GetDBValue() == "abcd");
    REQUIRE(field.SetObject(testObj) == false);
}

} // namespace FieldTypes
} // namespace Database
} // namespace Andromeda
