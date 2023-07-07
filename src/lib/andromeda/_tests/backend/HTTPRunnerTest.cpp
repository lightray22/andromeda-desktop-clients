#include "catch2/catch_test_macros.hpp"

#include "andromeda/backend/HTTPRunner.hpp"
#include "andromeda/backend/HTTPOptions.hpp"
#include "andromeda/backend/RunnerOptions.hpp"

namespace Andromeda {
namespace Backend {

class HTTPRunnerTest : public HTTPRunner
{ 
public:    
    using HTTPRunner::HTTPRunner;
    using HTTPRunner::RegisterRedirect;
};

/*****************************************************/
TEST_CASE("ParseURL", "[HTTPRunner]")
{
    HTTPRunner::HostUrlPair result {};

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
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};

    REQUIRE(HTTPRunner("myhost","","",ropts,hopts).GetHostname() == "myhost");
    REQUIRE(HTTPRunner("http://myhost","","",ropts,hopts).GetHostname() == "myhost");
    REQUIRE(HTTPRunner("https://myhost","","",ropts,hopts).GetHostname() == "myhost");
}

/*****************************************************/
TEST_CASE("GetProtoHost", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};

    REQUIRE(HTTPRunner("myhost","","",ropts,hopts).GetProtoHost() == "http://myhost");
    REQUIRE(HTTPRunner("http://myhost","","",ropts,hopts).GetProtoHost() == "http://myhost");
    REQUIRE(HTTPRunner("https://myhost","","",ropts,hopts).GetProtoHost() == "https://myhost");
}

/*****************************************************/
TEST_CASE("GetBaseURL", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};

    REQUIRE(HTTPRunner("","","",ropts,hopts).GetBaseURL() == "/");
    REQUIRE(HTTPRunner("","/","",ropts,hopts).GetBaseURL() == "/");
    REQUIRE(HTTPRunner("","test","",ropts,hopts).GetBaseURL() == "/test");
    REQUIRE(HTTPRunner("","/test","",ropts,hopts).GetBaseURL() == "/test");
    REQUIRE(HTTPRunner("","/?test","",ropts,hopts).GetBaseURL() == "/?test");
}

/*****************************************************/
TEST_CASE("EnableRetry", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};
    HTTPRunner runner("","","",ropts,hopts);

    REQUIRE(runner.GetCanRetry() == false); // default
    runner.EnableRetry(); REQUIRE(runner.GetCanRetry());
    runner.EnableRetry(false); REQUIRE(runner.GetCanRetry() == false);
}

/*****************************************************/
TEST_CASE("RegisterRedirect", "[HTTPRunner]")
{
    const HTTPOptions hopts {};
    const RunnerOptions ropts {};
    HTTPRunnerTest runner("myhost","/page","",ropts,hopts);

    runner.RegisterRedirect("http://mytest/page2");

    REQUIRE(runner.GetProtoHost() == "http://mytest");
    REQUIRE(runner.GetBaseURL() == "/page2");
}

} // namespace Backend
} // namespace Andromeda
