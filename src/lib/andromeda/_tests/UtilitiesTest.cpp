#include "catch2/catch_test_macros.hpp"

#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
TEST_CASE("Random", "[Utilities]")
{
    REQUIRE(Utilities::Random(0).empty());
    REQUIRE(Utilities::Random(1).size() == 1);
    REQUIRE(Utilities::Random(64).size() == 64);
    REQUIRE(Utilities::Random(65536).size() == 65536);
}

/*****************************************************/
TEST_CASE("explode", "[Utilities]")
{
    typedef std::vector<std::string> Strings; Strings result;
    
    result = Utilities::explode("","");
    REQUIRE(result.empty());

    result = Utilities::explode("","test");
    REQUIRE(result.empty());

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
    REQUIRE(result.first.empty()); REQUIRE(result.second.empty());

    result = Utilities::split("", "", 0, true);
    REQUIRE(result.first.empty()); REQUIRE(result.second.empty());
    
    result = Utilities::split("test", "");
    REQUIRE(result.first == "test"); REQUIRE(result.second.empty());
    
    result = Utilities::split("test", "", 0, true);
    REQUIRE(result.first.empty()); REQUIRE(result.second == "test");
    
    result = Utilities::split("test", "/");
    REQUIRE(result.first == "test"); REQUIRE(result.second.empty());
    
    result = Utilities::split("test", "/", 0, true);
    REQUIRE(result.first.empty()); REQUIRE(result.second == "test");
    
    result = Utilities::split("/test/", "/");
    REQUIRE(result.first.empty()); REQUIRE(result.second == "test/");
    
    result = Utilities::split("/test/", "/", 1);
    REQUIRE(result.first == "/test"); REQUIRE(result.second.empty());
    
    result = Utilities::split("/test/", "/", 0, true);
    REQUIRE(result.first == "/test"); REQUIRE(result.second.empty());
    
    result = Utilities::split("test1=test2=test3", "=");
    REQUIRE(result.first == "test1"); REQUIRE(result.second == "test2=test3");
    
    result = Utilities::split("test1=test2=test3=test4", "=", 1, true);
    REQUIRE(result.first == "test1=test2"); REQUIRE(result.second == "test3=test4");

    result = Utilities::split("folder1/folder2/file", "/", 0, true);
    REQUIRE(result.first == "folder1/folder2"); REQUIRE(result.second == "file");

    result = Utilities::split("http://mytest", "/", 2);
    REQUIRE(result.first == "http://mytest"); REQUIRE(result.second.empty());

    result = Utilities::split("http://mytest/test2", "/", 2);
    REQUIRE(result.first == "http://mytest"); REQUIRE(result.second == "test2");

    result = Utilities::split("http://mytest", "://");
    REQUIRE(result.first == "http"); REQUIRE(result.second == "mytest");
}

/*****************************************************/
TEST_CASE("startsWith", "[Utilities]")
{
    REQUIRE(Utilities::startsWith("",""));
    REQUIRE(Utilities::startsWith("a",""));
    REQUIRE(!Utilities::startsWith("","a"));

    REQUIRE(Utilities::startsWith("a","a"));
    REQUIRE(!Utilities::startsWith("a","b"));

    REQUIRE(Utilities::startsWith("test123","test"));
    REQUIRE(Utilities::startsWith("test123","test123"));
    REQUIRE(!Utilities::startsWith("test123","test1234"));
    REQUIRE(!Utilities::startsWith("test123"," test"));
    REQUIRE(!Utilities::startsWith("test123","123"));
}

/*****************************************************/
TEST_CASE("endsWith", "[Utilities]")
{
    REQUIRE(Utilities::endsWith("",""));
    REQUIRE(Utilities::endsWith("a",""));
    REQUIRE(!Utilities::endsWith("","a"));

    REQUIRE(Utilities::endsWith("a","a"));
    REQUIRE(!Utilities::endsWith("a","b"));

    REQUIRE(Utilities::endsWith("test123","123"));
    REQUIRE(Utilities::endsWith("test123","test123"));
    REQUIRE(!Utilities::endsWith("test123","test1234"));
    REQUIRE(!Utilities::endsWith("test123","123 "));
    REQUIRE(!Utilities::endsWith("test123","test"));
}

/*****************************************************/
TEST_CASE("trim", "[Utilities]") 
{
    REQUIRE(Utilities::trim("").empty());
    REQUIRE(Utilities::trim("test") == "test");
    REQUIRE(Utilities::trim(" test") == "test");
    REQUIRE(Utilities::trim("test1  ") == "test1");
    REQUIRE(Utilities::trim("\ttest\n") == "test");
    REQUIRE(Utilities::trim("test\ntest") == "test\ntest");
}

/*****************************************************/
TEST_CASE("replaceAll", "[Utilities]")
{
    REQUIRE(Utilities::replaceAll("","","").empty());
    REQUIRE(Utilities::replaceAll("","a","").empty());
    REQUIRE(Utilities::replaceAll("","a","b").empty());

    REQUIRE(Utilities::replaceAll("a","","") == "a");
    REQUIRE(Utilities::replaceAll("a","a","").empty());
    REQUIRE(Utilities::replaceAll("a","a","b") == "b");
    REQUIRE(Utilities::replaceAll("a","b","a") == "a");

    REQUIRE(Utilities::replaceAll("start,end","start","") == ",end");
    REQUIRE(Utilities::replaceAll("start,end","end","") == "start,");

    REQUIRE(Utilities::replaceAll("test,test2,test3,test4",",",",,") == "test,,test2,,test3,,test4");
    REQUIRE(Utilities::replaceAll("str\"thing\"str2","\"","\\\"") == "str\\\"thing\\\"str2");
}

/*****************************************************/
TEST_CASE("stringToBool", "[Utilities]") 
{
    REQUIRE(!Utilities::stringToBool(""));
    REQUIRE(!Utilities::stringToBool("0"));
    REQUIRE(!Utilities::stringToBool("false"));
    REQUIRE(!Utilities::stringToBool("off"));
    REQUIRE(!Utilities::stringToBool("no"));

    REQUIRE(Utilities::stringToBool("1"));
    REQUIRE(Utilities::stringToBool("true"));
    REQUIRE(Utilities::stringToBool("on"));
    REQUIRE(Utilities::stringToBool("yes"));

    REQUIRE(Utilities::stringToBool("test"));
}

/*****************************************************/
TEST_CASE("stringToBytes", "[Utilities]")
{
    REQUIRE(Utilities::stringToBytes("") == 0);
    REQUIRE(Utilities::stringToBytes(" ") == 0);
    REQUIRE(Utilities::stringToBytes("0") == 0);

    REQUIRE(Utilities::stringToBytes("1") == 1);
    REQUIRE(Utilities::stringToBytes(" 4567 ") == 4567);

    REQUIRE(Utilities::stringToBytes("1K") == 1024);
    REQUIRE(Utilities::stringToBytes(" 5 K ") == 5UL*1024);
    REQUIRE(Utilities::stringToBytes("256M") == 256UL*1024*1024);
    REQUIRE(Utilities::stringToBytes("2837483M") == 2837483ULL*1024*1024);
    REQUIRE(Utilities::stringToBytes("57G") == 57ULL*1024*1024*1024);
    REQUIRE(Utilities::stringToBytes("13T") == 13ULL*1024*1024*1024*1024);

    REQUIRE_THROWS_AS(Utilities::stringToBytes("R2D2"), std::logic_error);
}

/*****************************************************/
TEST_CASE("bytesToString", "[Utilities]")
{
    REQUIRE(Utilities::bytesToString(0) == "0");
    REQUIRE(Utilities::bytesToString(123) == "123");

    REQUIRE(Utilities::bytesToString(1024) == "1K");
    REQUIRE(Utilities::bytesToString(4096) == "4K");
    
    REQUIRE(Utilities::bytesToString(1048576) == "1M");
    REQUIRE(Utilities::bytesToString(1048577) == "1048577");
    REQUIRE(Utilities::bytesToString(1536000) == "1500K");
    
    REQUIRE(Utilities::bytesToString(45ULL*1024*1024*1024) == "45G");
    REQUIRE(Utilities::bytesToString(27ULL*1024*1024*1024*1024) == "27T");
    REQUIRE(Utilities::bytesToString(69ULL*1024*1024*1024*1024*1024) == "69P");
}

} // namespace Andromeda
