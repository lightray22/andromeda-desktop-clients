#include <string>
#include <iostream>
#include <utility>

#include <nlohmann/json.hpp>

#include "Backend.hpp"

Backend::Backend() : debug("Backend",this) { }

void Backend::Initialize()
{
    this->debug << __func__ << "()"; this->debug.Print();

    this->config.Initialize(*this);
}

nlohmann::json Backend::RunJsonAction(const std::string& app, const std::string& action)
{
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Print();

    std::string resp = RunAction(app, action);

    this->debug << __func__ << ": resp:" << resp; this->debug.Print();

    try
    {
        nlohmann::json val = nlohmann::json::parse(resp);

        this->debug << __func__ << ": json:" << val.dump(4); this->debug.Print();

        if (static_cast<int>(val["ok"])) 
            return val["appdata"];
        else
        {
            switch (static_cast<int>(val["code"]))
            {
                case 404: throw NotFoundException(val["message"]); break;
                default: throw APIException(val["code"], val["message"]); break;
            }
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw JSONErrorException(ex.what()); }
}

void Backend::Authenticate(const std::string& username)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Print();
//    std::cout << "Password?" << std::endl;
//    std::string password; std::getline(std::cin, password);
}

nlohmann::json Backend::GetServerConfig()
{
    // TODO load all 3 configs in one transaction
    return std::move(RunJsonAction("server","getconfig"));
}