
#include <array>
#include <filesystem>
#include <fstream>

#include "catch2/catch_test_macros.hpp"

#include "BaseOptions.hpp"
#include "TempPath.hpp"
#include "Utilities.hpp"

namespace Andromeda {

class TestOptions : public BaseOptions
{
public:
    bool AddFlag(const std::string& flag) override
    {
        flags.push_back(flag); return true;
    }

    bool AddOption(const std::string& option, const std::string& value) override
    {
        options.emplace(option, value); return true;
    }

    void TryAddUrlFlag(const std::string& flag) override
    {
        flags.push_back(flag);
    }

    void TryAddUrlOption(const std::string& option, const std::string& value) override
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
        const std::array<const char*,1> args { "test" };
        TestOptions options; options.ParseArgs(args.size(), args.data());
        REQUIRE(options.flags == BaseOptions::Flags{}); // NOLINT(readability-container-size-empty)
        REQUIRE(options.options == BaseOptions::Options{}); // NOLINT(readability-container-size-empty)
    }

    {
        const std::array<const char*,2> args { "test", "-d" };
        TestOptions options; options.ParseArgs(args.size(), args.data());
        REQUIRE(options.flags == BaseOptions::Flags{"d"});
        REQUIRE(options.options.empty());
    }

    {
        const std::array<const char*,12> args { "test", "-a", "-b1", "-c", "2", "-x=5", "--y=6", "--test", "--test2", "val", "--test3", "" };
        TestOptions options; options.ParseArgs(args.size(), args.data());
        REQUIRE(options.flags == BaseOptions::Flags{"a","test"});
        REQUIRE(options.options == BaseOptions::Options{{"b","1"},{"c","2"},{"x","5"},{"y","6"},{"test2","val"},{"test3",""}});
    }

    {
        const std::array<const char*,4> args { "test", "-a", "test1", "" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data()), BaseOptions::BadUsageException);
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data(), true), BaseOptions::BadUsageException);
    }

    {
        const std::array<const char*,4> args { "test", "-a", "test1", "test2" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data()), BaseOptions::BadUsageException);
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data(), true), BaseOptions::BadUsageException);
    }

    {
        const std::array<const char*,5> args { "test", "-a", "test1", "--", "test2" };
        TestOptions options; 
        REQUIRE_THROWS_AS(options.ParseArgs(args.size(), args.data()), BaseOptions::BadUsageException);
        REQUIRE(options.ParseArgs(args.size(), args.data(), true) == args.size()-1);
        REQUIRE(options.flags.empty());
        REQUIRE(options.options == BaseOptions::Options{{"a","test1"}});
    }
}

/*****************************************************/
void DoParseFile(TestOptions& options, const std::string& fileData)
{
    const TempPath tmppath("test_ParseFile"); 
    std::ofstream tmpfile;
    tmpfile.open(tmppath.Get()); 
    tmpfile << fileData; tmpfile.close();
    options.ParseFile(tmppath.Get());
}

TEST_CASE("ParseFile", "[BaseOptions]")
{
    {
        TestOptions options; DoParseFile(options, "");
        REQUIRE(options.flags.empty());
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; DoParseFile(options, "d");
        REQUIRE(options.flags == BaseOptions::Flags{"d"});
        REQUIRE(options.options.empty());
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
        REQUIRE(options.flags.empty());
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; options.ParseUrl("myhost/path?test");
        REQUIRE(options.flags == BaseOptions::Flags{"test"});
        REQUIRE(options.options.empty());
    }

    {
        TestOptions options; options.ParseUrl("https://test.com/path1/path2?test=&test2=a&b&c");
        REQUIRE(options.flags == BaseOptions::Flags{"b","c"});
        REQUIRE(options.options == BaseOptions::Options{{"test",""},{"test2","a"}});
    }
}

} // namespace Andromeda
