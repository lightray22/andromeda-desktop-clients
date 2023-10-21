
#include <string>

#include "catch2/catch_test_macros.hpp"

#include "andromeda/database/MixedValue.hpp"
#include "andromeda/database/QueryBuilder.hpp"

namespace Andromeda {
namespace Database {
namespace { // anonymous

/*****************************************************/
TEST_CASE("Compares", "[QueryBuilder]")
{
    {
        QueryBuilder q; q.Where(q.IsNull("mykey"));
        REQUIRE(q.GetParams().empty());
        REQUIRE("mykey IS NULL" == q.GetWhere());
    }

    {
        QueryBuilder q; q.Where(q.LessThan("mykey",5));
        REQUIRE(q.GetParams().size() == 1);
        REQUIRE(q.GetParams().at(":d0") == 5);
        REQUIRE(q.GetText() == "WHERE mykey < :d0");
    }

    {
        QueryBuilder q; q.Where(q.LessThanEquals("mykey",5));
        REQUIRE(q.GetParams().at(":d0") == 5);
        REQUIRE(q.GetText() == "WHERE mykey <= :d0");
    }

    {
        QueryBuilder q; q.Where(q.GreaterThan("mykey",5));
        REQUIRE(q.GetParams().at(":d0") == 5); 
        REQUIRE(q.GetText() == "WHERE mykey > :d0");
    }
    
    {
        QueryBuilder q; q.Where(q.GreaterThanEquals("mykey",5));
        REQUIRE(q.GetParams().at(":d0") == 5); 
        REQUIRE(q.GetText() == "WHERE mykey >= :d0");
    }

    {
        QueryBuilder q; q.Where(q.IsTrue("mykey"));
        REQUIRE(q.GetParams().at(":d0") == 0); 
        REQUIRE(q.GetText() == "WHERE mykey > :d0");
    }

    {
        QueryBuilder q; q.Where(q.Equals("mykey","myval"));
        REQUIRE(q.GetParams().at(":d0") == "myval"); 
        REQUIRE(q.GetText() == "WHERE mykey = :d0");
    }

    {
        QueryBuilder q; q.Where(q.Equals("mykey",nullptr));
        REQUIRE(q.GetParams().empty());
        REQUIRE(q.GetText() == "WHERE mykey IS NULL"); }

    {
        QueryBuilder q; q.Where(q.NotEquals("mykey","myval"));
        REQUIRE(q.GetParams().at(":d0") == "myval"); 
        REQUIRE(q.GetText() == "WHERE mykey <> :d0");
    }

    {
        QueryBuilder q; q.Where(q.Like("mykey","myval%",true));
        REQUIRE(q.GetParams().at(":d0") == "myval%"); 
        REQUIRE(q.GetText() == "WHERE mykey LIKE :d0 ESCAPE '\\'");
    }

    {
        QueryBuilder q; q.Where(q.Like("mykey","my_val\\_%"));
        REQUIRE(q.GetParams().at(":d0") == "%my\\_val\\\\\\_\\%%"); // EscapeWildcards
        REQUIRE(q.GetText() == "WHERE mykey LIKE :d0 ESCAPE '\\'");
    }
}

/*****************************************************/
TEST_CASE("Combos", "[QueryBuilder]")
{
    {
        QueryBuilder q; q.Where(q.Not(q.Equals("mykey","myval")));
        REQUIRE(q.GetParams().at(":d0") == "myval"); 
        REQUIRE(q.GetText() == "WHERE (NOT mykey = :d0)");
    }

    {
        QueryBuilder q; q.Where(q.NotEquals("mykey",nullptr));
        REQUIRE(q.GetParams().empty());
        REQUIRE(q.GetText() == "WHERE (NOT mykey IS NULL)");
    }

    {
        // argument evaluation order is unspecified in C++ so for the asserts to pass we have to control order
        QueryBuilder q; const std::string eq1 { q.Equals("mykey1","myval1") }; 
        q.Where(q.And(eq1, q.Equals("mykey2","myval2")));
        REQUIRE(q.GetParams().size() == 2);
        REQUIRE(q.GetParams().at(":d0") == "myval1");
        REQUIRE(q.GetParams().at(":d1") == "myval2");
        REQUIRE(q.GetText() == "WHERE (mykey1 = :d0 AND mykey2 = :d1)");
    }

    {
        // argument evaluation order is unspecified in C++ so for the asserts to pass we have to control order
        QueryBuilder q; const std::string eq1 { q.Equals("mykey1","myval1") };
        q.Where(q.Or(eq1, q.Equals("mykey2","myval2")));
        REQUIRE(q.GetParams().size() == 2);
        REQUIRE(q.GetParams().at(":d0") == "myval1");
        REQUIRE(q.GetParams().at(":d1") == "myval2");
        REQUIRE(q.GetText() == "WHERE (mykey1 = :d0 OR mykey2 = :d1)");
    }

    {
        QueryBuilder q; q.Where(q.ManyEqualsOr("mykey",{"myval1","myval2"}));
        REQUIRE(q.GetParams().size() == 2);
        REQUIRE(q.GetParams().at(":d0") == "myval1");
        REQUIRE(q.GetParams().at(":d1") == "myval2");
        REQUIRE(q.GetText() == "WHERE (mykey = :d0 OR mykey = :d1)");
    }
}

/*****************************************************/
TEST_CASE("AutoWhereAnd", "[QueryBuilder]")
{
    QueryBuilder q; q.Where(q.Equals("a",3)).Where(q.Equals("b",4));
    REQUIRE(q.GetParams().size() == 2);
    REQUIRE(q.GetParams().at(":d0") == 3);
    REQUIRE(q.GetParams().at(":d1") == 4);
    REQUIRE(q.GetText() == "WHERE (a = :d0 AND b = :d1)");
}

/*****************************************************/
TEST_CASE("Special", "[QueryBuilder]")
{
    QueryBuilder q; q.Where(q.IsNull("mykey"))
        .Limit(15).Offset(10).OrderBy("mykey",true);

    REQUIRE(*q.GetLimit() == 15);
    REQUIRE(*q.GetOffset() == 10);
    REQUIRE(q.GetOrderBy() == "mykey");
    REQUIRE(q.GetOrderDesc() == true);

    REQUIRE(q.GetParams().empty());
    REQUIRE(q.GetText() == "WHERE mykey IS NULL ORDER BY mykey DESC LIMIT 15 OFFSET 10");

    q.Where(nullptr).Limit(nullptr).Offset(nullptr).OrderBy(nullptr); // reset

    REQUIRE(q.GetLimit() == nullptr);
    REQUIRE(q.GetOffset() == nullptr);
    REQUIRE(q.GetOrderBy().empty());
    REQUIRE(q.GetOrderDesc() == false);

    REQUIRE(q.GetParams().empty());
    REQUIRE(q.GetText().empty());
}

} // namespace
} // namespace Database
} // namespace Andromeda
