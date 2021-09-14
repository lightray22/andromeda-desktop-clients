#include "Config.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

Config::Config(const nlohmann::json& data)
{
    std::cout << data["appdata"]["config"]["apps"]["server"] << std::endl; // TODO
}