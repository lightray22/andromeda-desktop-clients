#include "catch2/catch_test_macros.hpp"

#include "StringUtil.hpp"

namespace Andromeda {

/*****************************************************/
TEST_CASE("Random", "[StringUtil]")
{
    REQUIRE(StringUtil::Random(0).empty());
    
    REQUIRE(StringUtil::Random(1).size() == 1);
    REQUIRE(StringUtil::Random(64).size() == 64);
    REQUIRE(StringUtil::Random(65536).size() == 65536);

    REQUIRE(StringUtil::Random(65536).find('\0') == std::string::npos);
}

/*****************************************************/
TEST_CASE("implode", "[StringUtil]")
{
    std::vector<std::string> arr;
    REQUIRE(StringUtil::implode("",arr).empty());
    REQUIRE(StringUtil::implode("xyz",arr).empty());

    arr = {"test1"};
    REQUIRE(StringUtil::implode("",arr) == "test1");
    REQUIRE(StringUtil::implode("xyz",arr) == "test1");

    arr = {"test1","test2","test3"};
    REQUIRE(StringUtil::implode("",arr) == "test1test2test3");
    REQUIRE(StringUtil::implode("xyz",arr) == "test1xyztest2xyztest3");
}

/*****************************************************/
TEST_CASE("explode", "[StringUtil]")
{
    typedef std::vector<std::string> Strings; Strings result;
    
    result = StringUtil::explode("","");
    REQUIRE(result.empty());

    result = StringUtil::explode("","test");
    REQUIRE(result.empty());

    result = StringUtil::explode("test","");
    REQUIRE(result == Strings{"test"});

    result = StringUtil::explode("test","/");
    REQUIRE(result == Strings{"test"});

    result = StringUtil::explode("test/1/2/3test","/");
    REQUIRE(result == Strings{"test", "1", "2", "3test"});

    result = StringUtil::explode("//test//","/");
    REQUIRE(result == Strings{"", "", "test", "", ""});

    result = StringUtil::explode("//test//","//");
    REQUIRE(result == Strings{"", "test", ""});

    result = StringUtil::explode("test12testab12", "12");
    REQUIRE(result == Strings{"test", "testab", ""});

    result = StringUtil::explode("http://mytest","/", 2);
    REQUIRE(result == Strings{"http://mytest"});

    result = StringUtil::explode("/test1/page.php","/", 2, false, 2);
    REQUIRE(result == Strings{"/test1/page.php"});

    result = StringUtil::explode("http://mytest/test1/page.php","/", 2, false, 2);
    REQUIRE(result == Strings{"http://mytest", "test1/page.php"});

    result = StringUtil::explode("folder1/folder2/file", "/", 0, true, 2);
    REQUIRE(result == Strings{"folder1/folder2", "file"});

    result = StringUtil::explode("//folder1//folder2//file", "//", 1, true);
    REQUIRE(result == Strings{"", "folder1", "folder2//file"});
}

/*****************************************************/
TEST_CASE("split", "[StringUtil]")
{
    typedef std::pair<std::string,std::string> StrPair; StrPair result;

    result = StringUtil::split("", "");
    REQUIRE(result.first.empty()); REQUIRE(result.second.empty());

    result = StringUtil::split("", "", 0, true);
    REQUIRE(result.first.empty()); REQUIRE(result.second.empty());
    
    result = StringUtil::split("test", "");
    REQUIRE(result.first == "test"); REQUIRE(result.second.empty());
    
    result = StringUtil::split("test", "", 0, true);
    REQUIRE(result.first.empty()); REQUIRE(result.second == "test");
    
    result = StringUtil::split("test", "/");
    REQUIRE(result.first == "test"); REQUIRE(result.second.empty());
    
    result = StringUtil::split("test", "/", 0, true);
    REQUIRE(result.first.empty()); REQUIRE(result.second == "test");
    
    result = StringUtil::split("/test/", "/");
    REQUIRE(result.first.empty()); REQUIRE(result.second == "test/");
    
    result = StringUtil::split("/test/", "/", 1);
    REQUIRE(result.first == "/test"); REQUIRE(result.second.empty());
    
    result = StringUtil::split("/test/", "/", 0, true);
    REQUIRE(result.first == "/test"); REQUIRE(result.second.empty());
    
    result = StringUtil::split("test1=test2=test3", "=");
    REQUIRE(result.first == "test1"); REQUIRE(result.second == "test2=test3");
    
    result = StringUtil::split("test1=test2=test3=test4", "=", 1, true);
    REQUIRE(result.first == "test1=test2"); REQUIRE(result.second == "test3=test4");

    result = StringUtil::split("folder1/folder2/file", "/", 0, true);
    REQUIRE(result.first == "folder1/folder2"); REQUIRE(result.second == "file");

    result = StringUtil::split("http://mytest", "/", 2);
    REQUIRE(result.first == "http://mytest"); REQUIRE(result.second.empty());

    result = StringUtil::split("http://mytest/test2", "/", 2);
    REQUIRE(result.first == "http://mytest"); REQUIRE(result.second == "test2");

    result = StringUtil::split("http://mytest", "://");
    REQUIRE(result.first == "http"); REQUIRE(result.second == "mytest");
}

/*****************************************************/
TEST_CASE("splitPath", "[StringUtil]")
{
    using StringPair = StringUtil::StringPair;
    REQUIRE(StringUtil::splitPath("") == StringPair{"",""});
    REQUIRE(StringUtil::splitPath("/") == StringPair{"",""});
    REQUIRE(StringUtil::splitPath("a/") == StringPair{"","a"});
    REQUIRE(StringUtil::splitPath("/a") == StringPair{"","a"});
    REQUIRE(StringUtil::splitPath("a/b") == StringPair{"a","b"});
    REQUIRE(StringUtil::splitPath("a//b") == StringPair{"a/","b"});
    REQUIRE(StringUtil::splitPath("/a/b") == StringPair{"/a","b"});
    REQUIRE(StringUtil::splitPath("a/b/") == StringPair{"a","b"});
    REQUIRE(StringUtil::splitPath("/a/b/") == StringPair{"/a","b"});
    REQUIRE(StringUtil::splitPath("a/b/c") == StringPair{"a/b","c"});
    REQUIRE(StringUtil::splitPath("/a/b/c/") == StringPair{"/a/b","c"});
}

/*****************************************************/
TEST_CASE("startsWith", "[StringUtil]")
{
    REQUIRE(StringUtil::startsWith("",""));
    REQUIRE(StringUtil::startsWith("a",""));
    REQUIRE(!StringUtil::startsWith("","a"));

    REQUIRE(StringUtil::startsWith("a","a"));
    REQUIRE(!StringUtil::startsWith("a","b"));

    REQUIRE(StringUtil::startsWith("test123","test"));
    REQUIRE(StringUtil::startsWith("test123","test123"));
    REQUIRE(!StringUtil::startsWith("test123","test1234"));
    REQUIRE(!StringUtil::startsWith("test123"," test"));
    REQUIRE(!StringUtil::startsWith("test123","123"));
}

/*****************************************************/
TEST_CASE("endsWith", "[StringUtil]")
{
    REQUIRE(StringUtil::endsWith("",""));
    REQUIRE(StringUtil::endsWith("a",""));
    REQUIRE(!StringUtil::endsWith("","a"));

    REQUIRE(StringUtil::endsWith("a","a"));
    REQUIRE(!StringUtil::endsWith("a","b"));

    REQUIRE(StringUtil::endsWith("test123","123"));
    REQUIRE(StringUtil::endsWith("test123","test123"));
    REQUIRE(!StringUtil::endsWith("test123","test1234"));
    REQUIRE(!StringUtil::endsWith("test123","123 "));
    REQUIRE(!StringUtil::endsWith("test123","test"));
}

/*****************************************************/
TEST_CASE("trim", "[StringUtil]") 
{
    REQUIRE(StringUtil::trim("").empty());
    REQUIRE(StringUtil::trim("test") == "test");
    REQUIRE(StringUtil::trim(" test") == "test");
    REQUIRE(StringUtil::trim("test1  ") == "test1");
    REQUIRE(StringUtil::trim("\ttest\n") == "test");
    REQUIRE(StringUtil::trim("test\ntest") == "test\ntest");

    std::string s; StringUtil::trim_void(s); REQUIRE(s.empty());
    s = "test"; StringUtil::trim_void(s); REQUIRE(s == "test");
    s = " test"; StringUtil::trim_void(s); REQUIRE(s == "test");
    s = "test1  "; StringUtil::trim_void(s); REQUIRE(s == "test1");
    s = "\ttest\n"; StringUtil::trim_void(s); REQUIRE(s == "test");
    s = "test\ntest"; StringUtil::trim_void(s); REQUIRE(s == "test\ntest");
}

/*****************************************************/
TEST_CASE("tolower", "[StringUtil]")
{
    REQUIRE(StringUtil::tolower("").empty());
    REQUIRE(StringUtil::tolower("test") == "test");
    REQUIRE(StringUtil::tolower("MyTEsT1 aBc") == "mytest1 abc");
}

/*****************************************************/
TEST_CASE("replaceAll", "[StringUtil]")
{
    REQUIRE(StringUtil::replaceAll("","","").empty());
    REQUIRE(StringUtil::replaceAll("","a","").empty());
    REQUIRE(StringUtil::replaceAll("","a","b").empty());

    REQUIRE(StringUtil::replaceAll("a","","") == "a");
    REQUIRE(StringUtil::replaceAll("a","a","").empty());
    REQUIRE(StringUtil::replaceAll("a","a","b") == "b");
    REQUIRE(StringUtil::replaceAll("a","b","a") == "a");

    REQUIRE(StringUtil::replaceAll("start,end","start","") == ",end");
    REQUIRE(StringUtil::replaceAll("start,end","end","") == "start,");

    REQUIRE(StringUtil::replaceAll("test,test2,test3,test4",",",",,") == "test,,test2,,test3,,test4");
    REQUIRE(StringUtil::replaceAll("str\"thing\"str2","\"","\\\"") == "str\\\"thing\\\"str2");
}

/*****************************************************/
TEST_CASE("escapeAll", "[StringUtil]")
{
    REQUIRE(StringUtil::escapeAll("",{'a'},'b').empty());
    REQUIRE(StringUtil::escapeAll("a",{'a'},'r') == "ra");
    REQUIRE(StringUtil::escapeAll("\\",{'b'}) == "\\\\");

    // test _ __ ___ 2  ->  test \_ \_\_ \_\_\_ 2
    REQUIRE(StringUtil::escapeAll("test _ __ ___ 2",{'_'}) == "test \\_ \\_\\_ \\_\\_\\_ 2");

    // test __\__ 2  ->  test \_\_\\\_\_ 2
    REQUIRE(StringUtil::escapeAll("test __\\__ 2",{'_'}) == "test \\_\\_\\\\\\_\\_ 2");
    
    // test \_ \\_ \\\_ \\\\_ _%\%_ 2 -> test \\\_ \\\\\_ \\\\\\\_ \\\\\\\\\_ \_\%\\\%\_ 2
    REQUIRE(StringUtil::escapeAll("test \\_ \\\\_ \\\\\\_ \\\\\\\\_ __\\__ 2",{'_','%'},'\\')
        == "test \\\\\\_ \\\\\\\\\\_ \\\\\\\\\\\\\\_ \\\\\\\\\\\\\\\\\\_ \\_\\_\\\\\\_\\_ 2");
}

/*****************************************************/
TEST_CASE("stringToBool", "[StringUtil]") 
{
    REQUIRE(!StringUtil::stringToBool(""));
    REQUIRE(!StringUtil::stringToBool("0"));
    REQUIRE(!StringUtil::stringToBool("false"));
    REQUIRE(!StringUtil::stringToBool("off"));
    REQUIRE(!StringUtil::stringToBool("no"));

    REQUIRE(StringUtil::stringToBool("1"));
    REQUIRE(StringUtil::stringToBool("true"));
    REQUIRE(StringUtil::stringToBool("on"));
    REQUIRE(StringUtil::stringToBool("yes"));

    REQUIRE(StringUtil::stringToBool("test"));
}

/*****************************************************/
TEST_CASE("stringToBytes", "[StringUtil]")
{
    REQUIRE(StringUtil::stringToBytes("") == 0);
    REQUIRE(StringUtil::stringToBytes(" ") == 0);
    REQUIRE(StringUtil::stringToBytes("0") == 0);

    REQUIRE(StringUtil::stringToBytes("1") == 1);
    REQUIRE(StringUtil::stringToBytes(" 4567 ") == 4567);

    REQUIRE(StringUtil::stringToBytes("1K") == 1024);
    REQUIRE(StringUtil::stringToBytes(" 5 K ") == 5UL*1024);
    REQUIRE(StringUtil::stringToBytes("256M") == 256UL*1024*1024);
    REQUIRE(StringUtil::stringToBytes("2837483M") == 2837483ULL*1024*1024);
    REQUIRE(StringUtil::stringToBytes("57G") == 57ULL*1024*1024*1024);
    REQUIRE(StringUtil::stringToBytes("13T") == 13ULL*1024*1024*1024*1024);

    REQUIRE_THROWS_AS(StringUtil::stringToBytes("R2D2"), std::logic_error);
}

/*****************************************************/
TEST_CASE("bytesToString", "[StringUtil]")
{
    REQUIRE(StringUtil::bytesToString(0) == "0");
    REQUIRE(StringUtil::bytesToString(123) == "123");

    REQUIRE(StringUtil::bytesToString(1024) == "1K");
    REQUIRE(StringUtil::bytesToString(4096) == "4K");
    
    REQUIRE(StringUtil::bytesToString(1048576) == "1M");
    REQUIRE(StringUtil::bytesToString(1048577) == "1048577");
    REQUIRE(StringUtil::bytesToString(1536000) == "1500K");
    
    REQUIRE(StringUtil::bytesToString(45ULL*1024*1024*1024) == "45G");
    REQUIRE(StringUtil::bytesToString(27ULL*1024*1024*1024*1024) == "27T");
    REQUIRE(StringUtil::bytesToString(69ULL*1024*1024*1024*1024*1024) == "69P");
}

} // namespace Andromeda
