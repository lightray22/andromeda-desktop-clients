
#include <filesystem>
#include <fstream>

#include "catch2/catch_test_macros.hpp"

#include "BaseOptions.hpp"
#include "Utilities.hpp"

namespace Andromeda {

class TestOptions : public BaseOptions
{
public:
    virtual bool AddFlag(const std::string& flag) override
    {
        flags.push_back(flag); return true;
    }

    virtual bool AddOption(const std::string& option, const std::string& value) override
    {
        options.emplace(option, value); return true;
    }

    virtual void TryAddUrlFlag(const std::string& flag) override
    {
        flags.push_back(flag);
    }

    virtual void TryAddUrlOption(const std::string& option, const std::string& value) override
    {
        options.emplace(option, value);
    }

    BaseOptions::Flags flags;
    BaseOptions::Options options;
};

/*****************************************************/
TEST_CASE("ParseArgs", "[BaseOptions]")
{
    {
        const char* args[] { "test" };
        TestOptions options; options.ParseArgs(ARRSIZE(args), args);
        REQUIRE(options.flags == BaseOptions::Flags{});
        REQUIRE(options.options == BaseOptions::Options{});
    }

    {
        const char* args[] { "test", "-d" };
        TestOptions options; options.ParseArgs(ARRSIZE(args), args);
        REQUIRE(options.flags == BaseOptions::Flags{"d"});
        REQUIRE(options.options == BaseOptions::Options{});
    }

    {
        const char* args[] { "test", "-a", "-b1", "-c", "2", "-x=5", "--y=6", "--test", "--test2", "val", "--test3", "" };
        TestOptions options; options.ParseArgs(ARRSIZE(args), args);
        REQUIRE(options.flags == BaseOptions::Flags{"a","test"});
        REQUIRE(options.options == BaseOptions::Options{{"b","1"},{"c","2"},{"x","5"},{"y","6"},{"test2","val"},{"test3",""}});
    }

    {
        const char* args[] { "test", "-a", "test1", "" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(ARRSIZE(args), args), BaseOptions::BadUsageException);
        REQUIRE_THROWS_AS(options.ParseArgs(ARRSIZE(args), args, true), BaseOptions::BadUsageException);
    }

    {
        const char* args[] { "test", "-a", "test1", "test2" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(ARRSIZE(args), args), BaseOptions::BadUsageException);
        REQUIRE_THROWS_AS(options.ParseArgs(ARRSIZE(args), args, true), BaseOptions::BadUsageException);
    }

    {
        const char* args[] { "test", "-a", "test1", "--", "test2" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(ARRSIZE(args), args), BaseOptions::BadUsageException);
        REQUIRE(options.ParseArgs(ARRSIZE(args), args, true) == ARRSIZE(args)-1);
        REQUIRE(options.flags == BaseOptions::Flags{});
        REQUIRE(options.options == BaseOptions::Options{{"a","test1"}});
    }
}

/*****************************************************/
void DoParseFile(TestOptions& options, const std::string& fileData)
{
    std::string tmppath { std::filesystem::temp_directory_path().string()+"/a2_testParseFile" };
    std::ofstream tmpfile; tmpfile.open(tmppath); tmpfile << fileData; tmpfile.close();
    options.ParseFile(tmppath); std::filesystem::remove(tmppath);
}

TEST_CASE("ParseFile", "[BaseOptions]")
{
    {
        TestOptions options; DoParseFile(options, "");
        REQUIRE(options.flags == BaseOptions::Flags{});
        REQUIRE(options.options == BaseOptions::Options{});
    }

    {
        TestOptions options; DoParseFile(options, "d");
        REQUIRE(options.flags == BaseOptions::Flags{"d"});
        REQUIRE(options.options == BaseOptions::Options{});
    }

    {
        TestOptions options; DoParseFile(options, "a\n\n#test\nb=1\ntest=val\nccc\ntest2=\n");
        REQUIRE(options.flags == BaseOptions::Flags{"a","ccc"});
        REQUIRE(options.options == BaseOptions::Options{{"b","1"},{"test","val"},{"test2",""}});
    }
}

/*****************************************************/
TEST_CASE("ParseUrl", "[BaseOptions]")
{
    {
        TestOptions options; options.ParseUrl("");
        REQUIRE(options.flags == BaseOptions::Flags{});
        REQUIRE(options.options == BaseOptions::Options{});
    }

    {
        TestOptions options; options.ParseUrl("myhost/path?test");
        REQUIRE(options.flags == BaseOptions::Flags{"test"});
        REQUIRE(options.options == BaseOptions::Options{});
    }

    {
        TestOptions options; options.ParseUrl("https://test.com/path1/path2?test=&test2=a&b&c");
        REQUIRE(options.flags == BaseOptions::Flags{"b","c"});
        REQUIRE(options.options == BaseOptions::Options{{"test",""},{"test2","a"}});
    }
}

} // namespace Andromeda
