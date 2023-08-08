#include "catch2/catch_test_macros.hpp"

#include "andromeda/backend/HTTPRunner.hpp"
#include "andromeda/backend/HTTPOptions.hpp"
#include "andromeda/backend/RunnerOptions.hpp"

namespace Andromeda {
namespace Backend {

class HTTPRunnerTest : public HTTPRunner
{ 
public:    
    using HTTPRunner::HostUrlPair;
    using HTTPRunner::HTTPRunner;
    using HTTPRunner::ParseURL;
    using HTTPRunner::RegisterRedirect;
};

/*****************************************************/
TEST_CASE("ParseURL", "[HTTPRunner]")
{
    HTTPRunnerTest::HostUrlPair result {};

    result = HTTPRunnerTest::ParseURL("myhost");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunnerTest::ParseURL("myhost/");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunnerTest::ParseURL("myhost/test");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test");
    
    result = HTTPRunnerTest::ParseURL("myhost/test/");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test/");
    
    result = HTTPRunnerTest::ParseURL("myhost/test/index.php");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunnerTest::ParseURL("https://myhost");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunnerTest::ParseURL("https://myhost/");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunnerTest::ParseURL("http://myhost/test");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test");
    
    result = HTTPRunnerTest::ParseURL("https://myhost/test/");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/test/");
    
    result = HTTPRunnerTest::ParseURL("https://myhost/test/index.php");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/test/index.php");
}

/*****************************************************/
TEST_CASE("GetHostname", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};

    {
        const HTTPRunner runner("myhost/?test","",ropts,hopts);
        REQUIRE(runner.GetHostname() == "myhost");
        REQUIRE(runner.GetProtoHost() == "http://myhost");
        REQUIRE(runner.GetBaseURL() == "/?test");
        REQUIRE(runner.GetFullURL() == "http://myhost/?test");
    }

    {
        const HTTPRunner runner("http://myhost","",ropts,hopts);
        REQUIRE(runner.GetHostname() == "myhost");
        REQUIRE(runner.GetProtoHost() == "http://myhost");
        REQUIRE(runner.GetBaseURL() == "/");
        REQUIRE(runner.GetFullURL() == "http://myhost/");
    }
}

/*****************************************************/
TEST_CASE("EnableRetry", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};
    HTTPRunner runner("","",ropts,hopts);

    REQUIRE(runner.GetCanRetry() == false); // default
    runner.EnableRetry(); REQUIRE(runner.GetCanRetry());
    runner.EnableRetry(false); REQUIRE(runner.GetCanRetry() == false);
}

/*****************************************************/
TEST_CASE("RegisterRedirect", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};
    HTTPRunnerTest runner("myhost/page","",ropts,hopts);

    REQUIRE(runner.GetProtoHost() == "http://myhost");
    REQUIRE(runner.GetBaseURL() == "/page");

    runner.RegisterRedirect("http://mytest/page2");

    REQUIRE(runner.GetProtoHost() == "http://mytest");
    REQUIRE(runner.GetBaseURL() == "/page2");
}

} // namespace Backend
} // namespace Andromeda
