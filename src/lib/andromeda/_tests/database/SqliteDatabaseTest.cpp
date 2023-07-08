
#include <string>
#include <vector>

#include "catch2/catch_test_macros.hpp"

#include "andromeda/Debug.hpp" // TODO !! remove
#include "andromeda/TempPath.hpp"
#include "andromeda/database/MixedOutput.hpp"
#include "andromeda/database/SqliteDatabase.hpp"

namespace Andromeda {
namespace Database {

using RowVector = std::vector<SqliteDatabase::Row*>;
void ToVector(SqliteDatabase::RowList& in, RowVector& out)
{
    out.resize(in.size()); size_t i { 0 };
    for (SqliteDatabase::RowList::value_type& val : in)
        out[i++] = &val;
}

/*****************************************************/
TEST_CASE("Query", "[SqliteDatabase]")
{
    Debug::SetLevel(Debug::Level::INFO); // TODO !! remove

    const TempPath tmppath("test_sqlite.s3db"); 
    SqliteDatabase database(tmppath.Get());
    SqliteDatabase::RowList rows; RowVector rowsV;

    database.query("CREATE TABLE `mytest` (`id` INTEGER, `name` TEXT);",rows); REQUIRE(rows.empty());
    REQUIRE(database.query("INSERT INTO `mytest` VALUES (5, 'test1')",rows) == 1); REQUIRE(rows.empty());
    REQUIRE(database.query("INSERT INTO `mytest` VALUES (7, 'test2')",rows) == 1); REQUIRE(rows.empty());

    database.query("SELECT * FROM `mytest`",rows);
    REQUIRE(rows.size() == 2); ToVector(rows,rowsV);

    REQUIRE(static_cast<int>(rowsV[0]->at("id")) == 5);
    REQUIRE(static_cast<std::string>(rowsV[0]->at("name")) == "test1");
    REQUIRE(std::string(static_cast<const char*>(rowsV[0]->at("name"))) == "test1");

    REQUIRE(static_cast<int>(rowsV[1]->at("id")) == 7);
    const MixedOutput& name1 { rowsV[1]->at("name") };
    REQUIRE(name1.is_null() == false);
    REQUIRE(static_cast<std::string>(name1) == "test2");

    // note that only 1 column is modified, but 2 are matched, so retval is 2
    REQUIRE(database.query("UPDATE `mytest` SET `name` = 'test2'",rows) == 2); REQUIRE(rows.empty());
    REQUIRE(database.query("INSERT INTO `mytest` VALUES (9, NULL)",rows) == 1); REQUIRE(rows.empty());

    database.query("SELECT * FROM `mytest` WHERE `name` = 'test2'",rows);
    REQUIRE(rows.size() == 2); ToVector(rows,rowsV);
    REQUIRE(static_cast<int>(rowsV[0]->at("id")) == 5);
    REQUIRE(static_cast<std::string>(rowsV[0]->at("name")) == "test2"); // updated

    REQUIRE(database.query("DELETE FROM `mytest` WHERE `id` = 7",rows) == 1); REQUIRE(rows.empty());

    database.query("SELECT * FROM `mytest`",rows);
    REQUIRE(rows.size() == 2); ToVector(rows,rowsV);

    REQUIRE(static_cast<int>(rowsV[0]->at("id")) == 5);
    REQUIRE(static_cast<std::string>(rowsV[0]->at("name")) == "test2");

    REQUIRE(static_cast<int>(rowsV[1]->at("id")) == 9);
    const MixedOutput& name2 { rowsV[1]->at("name") };
    REQUIRE(name2.is_null() == true);
    REQUIRE(static_cast<const char*>(name2) == nullptr);
}

} // namespace Database
} // namespace Andromeda
