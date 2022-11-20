#include "catch2/catch_test_macros.hpp"

#include "HTTPRunner.hpp"

namespace Andromeda {

TEST_CASE("ParseURL", "[HTTPRunner]")
{
    HTTPRunner::HostUrlPair result;

    result = HTTPRunner::ParseURL("myhost");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/index.php");
    
    result = HTTPRunner::ParseURL("myhost/");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/index.php");
    
    result = HTTPRunner::ParseURL("myhost/test");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunner::ParseURL("myhost/test/");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunner::ParseURL("myhost/test/index.php");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunner::ParseURL("http://myhost");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/index.php");
    
    result = HTTPRunner::ParseURL("https://myhost");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/index.php");
    
    result = HTTPRunner::ParseURL("https://myhost/");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/index.php");
    
    result = HTTPRunner::ParseURL("http://myhost/test");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunner::ParseURL("https://myhost/test/");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunner::ParseURL("https://myhost/test/index.php");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/test/index.php");
}

} // namespace Andromeda
