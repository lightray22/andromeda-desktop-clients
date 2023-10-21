
#include <string>

#include "catch2/catch_test_macros.hpp"

#include "testObjects.hpp"
#include "andromeda/database/TableBuilder.hpp"

namespace Andromeda {
namespace Database {
namespace { // anonymous

/*****************************************************/
TEST_CASE("GetQueries", "[TableBuilder]")
{
    TableBuilder tb { TableBuilder::For<EasyObject>() };
    REQUIRE(tb.GetQueries().empty());

    tb.AddColumn("id","char(20)",false)
      .AddColumn("test1","integer",false)
      .AddColumn("test2","longtext",true)
      .SetPrimary("id")
      .AddUnique("test1")
      .AddUnique("test1","test2")
      .AddConstraint<EasyObject2>("test1","ref1")
      .AddConstraint<EasyObject2>("test2","ref2",TableBuilder::OnDelete::SET_NULL)
      .AddIndex("test1")
      .AddIndex("test1","test2");

    REQUIRE(tb.GetQueries() == std::list<std::string>{
        "CREATE TABLE `a2obj_database_easyobject` (`id` char(20) NOT NULL, `test1` integer NOT NULL, `test2` longtext DEFAULT NULL, "
            "PRIMARY KEY (`id`), UNIQUE (`test1`), UNIQUE (`test1`,`test2`), "
            "CONSTRAINT `a2obj_database_easyobject_ibfk_1` FOREIGN KEY (`test1`) REFERENCES `a2obj_database_easyobject2` (`ref1`) ON DELETE RESTRICT, "
            "CONSTRAINT `a2obj_database_easyobject_ibfk_2` FOREIGN KEY (`test2`) REFERENCES `a2obj_database_easyobject2` (`ref2`) ON DELETE SET NULL)",
        "CREATE INDEX \"idx_a2obj_database_easyobject_test1\" ON \"a2obj_database_easyobject\" (`test1`)", 
        "CREATE INDEX \"idx_a2obj_database_easyobject_test1_test2\" ON \"a2obj_database_easyobject\" (`test1`,`test2`)"
    });
}

} // namespace
} // namespace Database
} // namespace Andromeda
