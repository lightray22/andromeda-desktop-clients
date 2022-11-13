#include "catch2/catch_test_macros.hpp"

#include "Utilities.hpp"

namespace Andromeda {

TEST_CASE("explode", "[Utilities]")
{
    typedef std::vector<std::string> Strings; Strings result;
    
    result = Utilities::explode("","");
    REQUIRE(result == Strings{});

    result = Utilities::explode("","test");
    REQUIRE(result == Strings{});

    result = Utilities::explode("test","");
    REQUIRE(result == Strings{"test"});

    result = Utilities::explode("test","/");
    REQUIRE(result == Strings{"test"});

    result = Utilities::explode("test/1/2/3test","/");
    REQUIRE(result == Strings{"test", "1", "2", "3test"});

    result = Utilities::explode("//test//","/");
    REQUIRE(result == Strings{"", "", "test", "", ""});

    result = Utilities::explode("/test1/page.php","/", 2, 2);
    REQUIRE(result == Strings{"/test1/page.php"});

    result = Utilities::explode("http://mytest/test1/page.php","/", 2, 2);
    REQUIRE(result == Strings{"http://mytest", "test1/page.php"});
}

TEST_CASE("split", "[Utilities]")
{
    typedef std::pair<std::string,std::string> StrPair; StrPair result;

    result = Utilities::split("", "");
    REQUIRE(result.first == ""); REQUIRE(result.second == "");

    result = Utilities::split("", "", true);
    REQUIRE(result.first == ""); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "");
    REQUIRE(result.first == ""); REQUIRE(result.second == "test");
    
    result = Utilities::split("test", "", true);
    REQUIRE(result.first == "test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "/");
    REQUIRE(result.first == "test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "/", true);
    REQUIRE(result.first == ""); REQUIRE(result.second == "test");
    
    result = Utilities::split("/test/", "/");
    REQUIRE(result.first == ""); REQUIRE(result.second == "test/");
    
    result = Utilities::split("/test/", "/", true);
    REQUIRE(result.first == "/test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test1=test2=test3", "=");
    REQUIRE(result.first == "test1"); REQUIRE(result.second == "test2=test3");
    
    result = Utilities::split("test1=test2=test3", "=", true);
    REQUIRE(result.first == "test1=test2"); REQUIRE(result.second == "test3");
}

TEST_CASE("endsWith", "[Utilities]")
{
    REQUIRE(Utilities::endsWith("","") == true);
    REQUIRE(Utilities::endsWith("a","") == true);
    REQUIRE(Utilities::endsWith("","a") == false);

    REQUIRE(Utilities::endsWith("a","a") == true);
    REQUIRE(Utilities::endsWith("a","b") == false);

    REQUIRE(Utilities::endsWith("test123","123") == true);
    REQUIRE(Utilities::endsWith("test123","123 ") == false);
    REQUIRE(Utilities::endsWith("test123","test") == false);
}

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
