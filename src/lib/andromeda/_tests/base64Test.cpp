
#include "catch2/catch_test_macros.hpp"

#include "base64.hpp"

namespace Andromeda {

/*****************************************************/
TEST_CASE("Encode", "base64")
{
    REQUIRE(base64::encode("").empty());

    REQUIRE(base64::encode("a") == "YQ=="); // 2==
    REQUIRE(base64::encode("ab") == "YWI="); // 1==
    REQUIRE(base64::encode("abc") == "YWJj"); // 0==

    REQUIRE(base64::encode("What's in a name? That which we call a rose By any other word would smell as sweet.")
        == "V2hhdCdzIGluIGEgbmFtZT8gVGhhdCB3aGljaCB3ZSBjYWxsIGEgcm9zZSBCeSBhbnkgb3RoZXIgd29yZCB3b3VsZCBzbWVsbCBhcyBzd2VldC4=");

    const std::array<char, 8> dat { '\x10','\x00','\x21','\xD0','\x9C','\x61','\xFF','\x46' };
    const std::string str(dat.data(), dat.size());
    REQUIRE(base64::encode(str) == "EAAh0Jxh/0Y=");
}

} // namespace Andromeda
