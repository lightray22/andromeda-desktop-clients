#include "Backend.hpp"
#include "Runner.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <iostream>
#include <utility>

Backend::Backend(Runner& runner) : runner(runner),
    config(DecodeResponse(runner.RunAction("server", "getconfig")))
{

}

nlohmann::json Backend::DecodeResponse(const std::string& response)
{
    std::cout << "decode: " << response << std::endl;

    nlohmann::json val;
    
    try
    {
        val = nlohmann::json::parse(response);
        std::cout << val.dump(4) << std::endl;
        // TODO do our usual andromeda response checks

        return std::move(val);
    }
    catch(const nlohmann::json::exception& ex)
    {
        std::cout << "Exception: " << ex.what() << std::endl;
        return "???"; // TODO rethrow our own exception here
    }
}

void Backend::Authenticate(const std::string& username)
{
//   std::cout << "Password?" << std::endl;
//    std::string password; std::getline(std::cin, password);
}
