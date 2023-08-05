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

    const EasyObject* obj { objdb.TryLoadUniqueByQuery<EasyObject>(q) };

    REQUIRE(obj != nullptr);
    REQUIRE(obj->ID() == "abc");
    REQUIRE(*obj->getMyStr() == "test1");

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 ==  "SELECT * FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}}).RETURN(0UL);
    
    REQUIRE(objdb.TryLoadUniqueByQuery<EasyObject>(q) == nullptr);
}

/*****************************************************/
TEST_CASE("SaveObject", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    EasyObject& obj { EasyObject::Create(objdb, 8) };
    const std::string id { obj.ID() };
    REQUIRE(objdb.getLoadedCount() == 0);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "INSERT INTO a2obj_database_easyobject (id,myint) VALUES (:d0,:d1)")
        .WITH(_2 == MixedParams{{":d0",id},{":d1",8}}).RETURN(1UL);

    obj.Save();
    REQUIRE(objdb.getLoadedCount() == 1);
    obj.Save(); // no-op

    obj.setMyStr("test123");
    
    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "UPDATE a2obj_database_easyobject SET mystr=:d0 WHERE id=:id")
        .WITH(_2 == MixedParams{{":d0","test123"},{":id",id}}).RETURN(1UL);

    obj.Save();
    obj.Save(); // no-op
}

/*****************************************************/
TEST_CASE("SaveAllObjects", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    EasyObject& obj1 { EasyObject::Create(objdb, 3) };
    const std::string id1 { obj1.ID() };

    EasyObject obj2(objdb, MixedParams{{"id","obj2"},{"myint",2}});
    obj2.setMyStr("test2"); // modify

    obj1.deltaCounter(1); 
    obj2.deltaCounter(2); // test counter

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "INSERT INTO a2obj_database_easyobject (id,myctr,myint) VALUES (:d0,:d1,:d2)")
        .WITH(_2 == MixedParams{{":d0",id1},{":d1",1},{":d2",3}}).RETURN(1UL);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "UPDATE a2obj_database_easyobject SET myctr=myctr+:d0, mystr=:d1 WHERE id=:id")
        .WITH(_2 == MixedParams{{":d0",2},{":d1","test2"},{":id","obj2"}}).RETURN(1UL);

    REQUIRE(objdb.getLoadedCount() == 0);
    objdb.SaveObjects();
    REQUIRE(objdb.getLoadedCount() == 1); // created!
}

/*****************************************************/
TEST_CASE("DeleteObject", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    EasyObject& obj1 { EasyObject::Create(objdb, 1) };

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject ").WITH(_2 == MixedParams{})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abc"},{"myint",33}})).RETURN(0UL);

    EasyObject* obj2 { objdb.TryLoadUniqueByQuery<EasyObject>({}) };
    REQUIRE(obj2 != nullptr);
    REQUIRE(objdb.getLoadedCount() == 1);

    bool obj1Del { false };
    bool obj2Del { false };
    obj1.onDelete([&](){ obj1Del = true; });
    obj2->onDelete([&](){ obj2Del = true; });

    objdb.DeleteObject(obj1);
    REQUIRE(obj1Del == true);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "DELETE FROM a2obj_database_easyobject WHERE id=:id")
        .WITH(_2 == MixedParams{{":id","abc"}}).RETURN(1UL);

    objdb.DeleteObject(*obj2);
    REQUIRE(obj2Del == true);
    REQUIRE(objdb.getLoadedCount() == 0);
}

/*****************************************************/
TEST_CASE("DeleteByQuery", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    QueryBuilder q; q.Where(q.Equals("myint",5));

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abc"},{"myint",5}}))
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","xyz"},{"myint",5}})).RETURN(0UL);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "DELETE FROM a2obj_database_easyobject WHERE id=:id")
        .WITH(_2 == MixedParams{{":id","abc"}}).RETURN(1UL);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "DELETE FROM a2obj_database_easyobject WHERE id=:id")
        .WITH(_2 == MixedParams{{":id","xyz"}}).RETURN(1UL);

    REQUIRE(objdb.DeleteObjectsByQuery<EasyObject>(q) == 2);
}

/*****************************************************/
TEST_CASE("DeleteUnique", "[ObjectDatabase]")
{
    MockSqliteDatabase sqldb;
    ObjectDatabase objdb(sqldb);

    QueryBuilder q; q.Where(q.Equals("myint",5));

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "SELECT * FROM a2obj_database_easyobject WHERE myint = :d0")
        .WITH(_2 == MixedParams{{":d0",5}})
        .SIDE_EFFECT(_3.emplace_back(Row{{"id","abc"},{"myint",5}})).RETURN(0UL);

    REQUIRE_CALL(sqldb, query(_,_,_))
        .WITH(_1 == "DELETE FROM a2obj_database_easyobject WHERE id=:id")
        .WITH(_2 == MixedParams{{":id","abc"}}).RETURN(1UL);

    REQUIRE(objdb.TryDeleteUniqueByQuery<EasyObject>(q) == true);
}

} // namespace Database
} // namespace Andromeda
