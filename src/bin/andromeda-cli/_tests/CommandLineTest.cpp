
#include <filesystem>
#include <fstream>

#include "catch2/catch_test_macros.hpp"

#include "CommandLine.hpp"

namespace Andromeda {

/*****************************************************/
TEST_CASE("ParseFullArgs", "[CommandLine]")
{
    /*{
        const char* args[] { "test", "-a", "test1", "test2" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(4, args), CommandLine::BadUsageException);
        REQUIRE(options.ParseArgs(4, args, true) == 3);
        REQUIRE(options.flags == CommandLine::Flags{});
        REQUIRE(options.options == CommandLine::Options{{"a","test1"}});
    }*/
}

} // namespace Andromeda
