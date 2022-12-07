#include "catch2/catch_test_macros.hpp"

#include "HTTPRunner.hpp"
#include "HTTPRunnerOptions.hpp"

namespace Andromeda {

class HTTPRunnerFriend
{
public:
    HTTPRunnerFriend(HTTPRunner& runner) : mRunner(runner) { }

    virtual ~HTTPRunnerFriend() { }

    virtual void HandleRedirect(const std::string& location) { mRunner.HandleRedirect(location); }

    HTTPRunner& mRunner;
};

/*****************************************************/
TEST_CASE("ParseURL", "[HTTPRunner]")
{
    HTTPRunner::HostUrlPair result;

    result = HTTPRunner::ParseURL("myhost");
    REQUIRE(result.first == "myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunner::ParseURL("myhost/");
    REQUIRE(result.first == "myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunner::ParseURL("myhost/test");
    REQUIRE(result.first == "myhost"); 
    REQUIRE(result.second == "/test");
    
    result = HTTPRunner::ParseURL("myhost/test/");
    REQUIRE(result.first == "myhost"); 
    REQUIRE(result.second == "/test/");
    
    result = HTTPRunner::ParseURL("myhost/test/index.php");
    REQUIRE(result.first == "myhost"); 
    REQUIRE(result.second == "/test/index.php");
    
    result = HTTPRunner::ParseURL("https://myhost");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunner::ParseURL("https://myhost/");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/");
    
    result = HTTPRunner::ParseURL("http://myhost/test");
    REQUIRE(result.first == "http://myhost"); 
    REQUIRE(result.second == "/test");
    
    result = HTTPRunner::ParseURL("https://myhost/test/");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/test/");
    
    result = HTTPRunner::ParseURL("https://myhost/test/index.php");
    REQUIRE(result.first == "https://myhost"); 
    REQUIRE(result.second == "/test/index.php");
}

/*****************************************************/
TEST_CASE("GetHostname", "[HTTPRunner]")
{
    const HTTPRunnerOptions options;

    REQUIRE(HTTPRunner("myhost","",options).GetHostname() == "myhost");
    REQUIRE(HTTPRunner("http://myhost","",options).GetHostname() == "myhost");
    REQUIRE(HTTPRunner("https://myhost","",options).GetHostname() == "myhost");
}

/*****************************************************/
TEST_CASE("GetProtoHost", "[HTTPRunner]")
{
    const HTTPRunnerOptions options;

    REQUIRE(HTTPRunner("myhost","",options).GetProtoHost() == "http://myhost");
    REQUIRE(HTTPRunner("http://myhost","",options).GetProtoHost() == "http://myhost");
    REQUIRE(HTTPRunner("https://myhost","",options).GetProtoHost() == "https://myhost");
}

/*****************************************************/
TEST_CASE("GetBaseURL", "[HTTPRunner]")
{
    const HTTPRunnerOptions options;

    REQUIRE(HTTPRunner("","",options).GetBaseURL() == "/");
    REQUIRE(HTTPRunner("","/",options).GetBaseURL() == "/");
    REQUIRE(HTTPRunner("","test",options).GetBaseURL() == "/test");
    REQUIRE(HTTPRunner("","/test",options).GetBaseURL() == "/test");
    REQUIRE(HTTPRunner("","/?test",options).GetBaseURL() == "/?test");
}

/*****************************************************/
TEST_CASE("EnableRetry", "[HTTPRunner]")
{
    const HTTPRunnerOptions options;
    HTTPRunner runner("", "", options);

    REQUIRE(runner.GetCanRetry() == false); // default
    runner.EnableRetry(); REQUIRE(runner.GetCanRetry() == true);
    runner.EnableRetry(false); REQUIRE(runner.GetCanRetry() == false);
}

/*****************************************************/
TEST_CASE("HandleRedirect", "[HTTPRunner]")
{
    const HTTPRunnerOptions options;
    HTTPRunner runner("myhost", "/page", options);
    HTTPRunnerFriend runnerFriend(runner);

    runnerFriend.HandleRedirect("http://mytest/page2");

    REQUIRE(runner.GetProtoHost() == "http://mytest");
    REQUIRE(runner.GetBaseURL() == "/page2");
}

} // namespace Andromeda
