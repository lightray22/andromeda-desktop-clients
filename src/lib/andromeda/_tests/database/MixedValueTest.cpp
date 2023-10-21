
#include <string>

#include "catch2/catch_test_macros.hpp"

#include "andromeda/database/MixedValue.hpp"

namespace Andromeda {
namespace Database {
namespace { // anonymous

/*****************************************************/
TEST_CASE("MixedValue", "[MixedValue]")
{
    const MixedValue a { 5 };
    const MixedValue b { a }; // copy // NOLINT(*-unnecessary-copy-initialization)
    const MixedValue c { "5" };
    const MixedValue d { nullptr };
    const MixedValue e { 6 };

    REQUIRE(a == a);
    REQUIRE(b == a);
    REQUIRE(c != a);
    REQUIRE(d != a);
    REQUIRE(e != a);

    REQUIRE(a.ToString() == "5");
    REQUIRE(c.ToString() == "5");
    REQUIRE(d.ToString() == "NULL");

    const std::string sa { "test" };
    const MixedValue f { sa };
    const std::string sb { "test" };
    const MixedValue g { sb };

    // same string, different pointers
    REQUIRE(f == g);

    const MixedParams ma {{"a",5},{"b","test"}};
    const MixedParams mb = ma; // copy // NOLINT(*-unnecessary-copy-initialization)
    const MixedParams mc {{"a",6},{"b","test"}};
    const MixedParams md {{"a",5},{"b","test2"}};

    REQUIRE(ma == ma);
    REQUIRE(ma == mb);
    REQUIRE(ma != mc);
    REQUIRE(ma != md);
}

} // namespace
} // namespace Database
} // namespace Andromeda
