#include "catch2/catch_test_macros.hpp"
#include "catch2/trompeloeil.hpp"

#include "testObjects.hpp"
#include "andromeda/database/ObjectDatabase.hpp"
#include "andromeda/database/QueryBuilder.hpp"
#include "andromeda/database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

using trompeloeil::_;
using Row = SqliteDatabase::Row;
using RowList = SqliteDatabase::RowList;

/*****************************************************/
TEST_CASE("GetClassTable", "[ObjectDatabase]")
{
    REQUIRE(ObjectDatabase::GetClassTableName("Andromeda\\Database\\EasyObject") == "a2obj_database_easyobject");
}

/*****************************************************/
TEST_CASE("CountByQuery", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    QueryBuilder q; q.Where(q.Equals("myint",5));

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT COUNT(id) FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"COUNT(id)",3}})).RETURN(0UL);

    REQUIRE(objdb.CountObjectsByQuery<EasyObject>(q) == 3);
}

/*****************************************************/
TEST_CASE("LoadByQuery", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    QueryBuilder q; q.Where(q.Equals("myint",5));

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abc"},{"myint",5},{"mystr","test1"}}))
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","xyz"},{"myint",5},{"mystr",nullptr}})).RETURN(0UL);

    const std::list<EasyObject*> objs { objdb.LoadObjectsByQuery<EasyObject>(q) };

    REQUIRE(objs.size() == 2); auto it { objs.begin() };
    const EasyObject& obj1 { **(it++) };
    const EasyObject& obj2 { **(it++) };

    REQUIRE(obj1.ID() == "abc");
    REQUIRE(*obj1.getMyStr() == "test1");
    REQUIRE(obj2.ID() == "xyz");
    REQUIRE(obj2.getMyStr() == nullptr);
}

/*****************************************************/
TEST_CASE("ObjectIdentity", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    const QueryBuilder q; // no WHERE

    REQUIRE_CALL(sqldb, query(_,_,_)).TIMES(2)
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject ")
        .WITH(_2 == MixedParams{})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abc"},{"myint",5}})).RETURN(0UL);

    const std::list<EasyObject*> objs1 { objdb.LoadObjectsByQuery<EasyObject>(q) };
    const std::list<EasyObject*> objs2 { objdb.LoadObjectsByQuery<EasyObject>(q) };

    REQUIRE(objs1.size() == 1); const EasyObject* obj1 { objs1.front() };
    REQUIRE(objs2.size() == 1); const EasyObject* obj2 { objs2.front() };
    REQUIRE(objdb.getLoadedCount() == 1);

    // Test that loading the same object twice does not reconstruct it
    REQUIRE(obj1 == obj2); // same ptr
    REQUIRE(*obj1 == *obj2); // operator==
    REQUIRE(obj1->ID() == obj2->ID());
}

/*****************************************************/
TEST_CASE("LoadUnique", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    QueryBuilder q; q.Where(q.Equals("myint",5));

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 ==  "SELECT * FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abc"},{"myint",5},{"mystr","test1"}})).RETURN(0UL);

    EasyObject* obj { objdb.TryLoadUniqueByQuery<EasyObject>(q) };

    REQUIRE(obj != nullptr);
    REQUIRE(obj->ID() == "abc");
    REQUIRE(*obj->getMyStr() == "test1");

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 ==  "SELECT * FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}}).RETURN(0UL);
    
    REQUIRE(objdb.TryLoadUniqueByQuery<EasyObject>(q) == nullptr);
}

/*****************************************************/
TEST_CASE("InsertObject", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    EasyObject& obj { EasyObject::Create(objdb, 8) };
    const std::string id { obj.ID() };
    REQUIRE(objdb.getLoadedCount() == 0);

    REQUIRE_CALL(sqldb, query(_,_))
        .WITH(_1 == "INSERT INTO a2obj_database_easyobject (myint,id) VALUES (:d0,:d1)")
        .WITH(_2 == MixedParams{{":d0",8},{":d1",id}}).RETURN(1UL);

    obj.Save();
    REQUIRE(objdb.getLoadedCount() == 1);
}

// TODO !! unit tests

/*****************************************************/
TEST_CASE("UpdateObject", "[ObjectDatabase]")
{

}

/*****************************************************/
TEST_CASE("SaveAllObjects", "[ObjectDatabase]")
{

}

/*****************************************************/
TEST_CASE("DeleteByQuery", "[ObjectDatabase]")
{
    /*MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    QueryBuilder q; q.Where(q.Equals("myint",5));

    objdb.DeleteObjectsByQuery<EasyObject>(q);*/
}

/*****************************************************/
TEST_CASE("DeleteObject", "[ObjectDatabase]")
{
    // TODO also test DeleteByQuery?
}

/*****************************************************/
TEST_CASE("DeleteUnique", "[ObjectDatabase]")
{

}

} // namespace Database
} // namespace Andromeda
