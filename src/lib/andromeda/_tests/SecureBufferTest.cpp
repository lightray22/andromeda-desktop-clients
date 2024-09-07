
#include <sodium.h>
#include "catch2/catch_test_macros.hpp"

#include "SecureBuffer.hpp"

namespace Andromeda {
namespace { // anonymous

/*****************************************************/
TEST_CASE("SecureMemory","[SecureBuffer]")
{
    const std::string testStr { "this is a test string" };
    char* secMem = SecureMemory::allocT<char>(testStr.size()+1);
    memcpy(secMem, testStr.c_str(), testStr.size()+1);
    REQUIRE(testStr == secMem);
    SecureMemory::freeT(secMem);
}

/*****************************************************/
TEST_CASE("SecureBuffer","[SecureBuffer]")
{
    SecureBuffer s0(static_cast<size_t>(0));
    REQUIRE(s0.data() != nullptr);
    REQUIRE(s0.size() == 0);

    SecureBuffer s(4);
    REQUIRE(s.data() != nullptr);
    REQUIRE(s.size() == 4);
    memcpy(s.data(), "test", 4);

    const SecureBuffer s2(s); // copy
    REQUIRE(s == s2); // operator==

    const SecureBuffer s3 { SecureBuffer::Insecure_FromCstr("test") }; // c_str
    REQUIRE(s == s3);

    SecureBuffer s4(std::move(s)); // move
    REQUIRE(s3 == s4);

    s4.resize(10);
    REQUIRE(s4.size() == 10);
    REQUIRE(s4.substr(0,4) == "test");
    REQUIRE(s4.substr(2,2) == "st");

    s4.resize(3);
    const SecureBuffer s4b { SecureBuffer::Insecure_FromCstr("tes") };
    REQUIRE(s4 == s4b);
}

} // namespace
} // namespace Andromeda
