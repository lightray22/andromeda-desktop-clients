
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

    const std::array<char, 8> dat { 
        static_cast<char>(0x10),
        static_cast<char>(0x00),
        static_cast<char>(0x21),
        static_cast<char>(0xD0),
        static_cast<char>(0x9C),
        static_cast<char>(0x61),
        static_cast<char>(0xFF),
        static_cast<char>(0x46),
    };
    const std::string str(dat.data(), dat.size());
    REQUIRE(base64::encode(str) == "EAAh0Jxh/0Y=");
}

} // namespace Andromeda
