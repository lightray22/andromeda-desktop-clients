#include "catch2/catch_test_macros.hpp"

#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
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

    result = Utilities::explode("//test//","//");
    REQUIRE(result == Strings{"", "test", ""});

    result = Utilities::explode("test12testab12", "12");
    REQUIRE(result == Strings{"test", "testab", ""});

    result = Utilities::explode("http://mytest","/", 2);
    REQUIRE(result == Strings{"http://mytest"});

    result = Utilities::explode("/test1/page.php","/", 2, false, 2);
    REQUIRE(result == Strings{"/test1/page.php"});

    result = Utilities::explode("http://mytest/test1/page.php","/", 2, false, 2);
    REQUIRE(result == Strings{"http://mytest", "test1/page.php"});

    result = Utilities::explode("folder1/folder2/file", "/", 0, true, 2);
    REQUIRE(result == Strings{"folder1/folder2", "file"});

    result = Utilities::explode("//folder1//folder2//file", "//", 1, true);
    REQUIRE(result == Strings{"", "folder1", "folder2//file"});
}

/*****************************************************/
TEST_CASE("split", "[Utilities]")
{
    typedef std::pair<std::string,std::string> StrPair; StrPair result;

    result = Utilities::split("", "");
    REQUIRE(result.first == ""); REQUIRE(result.second == "");

    result = Utilities::split("", "", 0, true);
    REQUIRE(result.first == ""); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "");
    REQUIRE(result.first == "test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "", 0, true);
    REQUIRE(result.first == "test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "/");
    REQUIRE(result.first == "test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test", "/", 0, true);
    REQUIRE(result.first == "test"); REQUIRE(result.second == "");
    
    result = Utilities::split("/test/", "/");
    REQUIRE(result.first == ""); REQUIRE(result.second == "test/");
    
    result = Utilities::split("/test/", "/", 1);
    REQUIRE(result.first == "/test"); REQUIRE(result.second == "");
    
    result = Utilities::split("/test/", "/", 0, true);
    REQUIRE(result.first == "/test"); REQUIRE(result.second == "");
    
    result = Utilities::split("test1=test2=test3", "=");
    REQUIRE(result.first == "test1"); REQUIRE(result.second == "test2=test3");
    
    result = Utilities::split("test1=test2=test3=test4", "=", 1, true);
    REQUIRE(result.first == "test1=test2"); REQUIRE(result.second == "test3=test4");

    result = Utilities::split("folder1/folder2/file", "/", 0, true);
    REQUIRE(result.first == "folder1/folder2"); REQUIRE(result.second == "file");

    result = Utilities::split("http://mytest", "/", 2);
    REQUIRE(result.first == "http://mytest"); REQUIRE(result.second == "");

    result = Utilities::split("http://mytest/test2", "/", 2);
    REQUIRE(result.first == "http://mytest"); REQUIRE(result.second == "test2");

    result = Utilities::split("http://mytest", "://");
    REQUIRE(result.first == "http"); REQUIRE(result.second == "mytest");
}

/*****************************************************/
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

/*****************************************************/
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

// TODO test parseArgs, parseFile, parseUrl

} // namespace Andromeda
