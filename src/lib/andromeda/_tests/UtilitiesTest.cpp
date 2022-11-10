#include "catch2/catch_test_macros.hpp"

#include "Utilities.hpp"

namespace Andromeda {

TEST_CASE("stringToBool", "[Utilities]") 
{
    REQUIRE(Utilities::stringToBool("") == false);
    REQUIRE(Utilities::stringToBool("0") == false);
    REQUIRE(Utilities::stringToBool("false") == false);
    REQUIRE(Utilities::stringToBool("off") == false);
    REQUIRE(Utilities::stringToBool("no") == false);

    REQUIRE(Utilities::stringToBool("1") == true);
    REQUIRE(Utilities::stringToBool("true") == true);
    REQUIRE(Utilities::stringToBool("on") == true);
    REQUIRE(Utilities::stringToBool("yes") == true);

    REQUIRE(Utilities::stringToBool("test") == true);
}

} // namespace Andromeda
