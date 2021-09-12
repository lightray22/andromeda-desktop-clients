
#include "Backend.hpp"

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <iostream>

Backend::Backend(const std::string& apiLocation, const std::string& username) :
    apiLocation(apiLocation)
{
    std::cout << "Password?" << std::endl;
    std::string password; std::getline(std::cin, password);
}