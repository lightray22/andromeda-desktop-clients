#include <string>
#include <iostream>
#include <utility>
#include <sstream>

#include <nlohmann/json.hpp>

#include "Backend.hpp"

Backend::Backend() : debug("Backend",this) { }

void Backend::Initialize()
{
    this->debug << __func__ << "()"; this->debug.Print();

    this->config.Initialize(*this);
}

std::string Backend::RunBinaryAction(const std::string& app, const std::string& action, const Params& params)
{
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Print();

    return std::move(RunAction(app, action, params));
}

nlohmann::json Backend::RunJsonAction(const std::string& app, const std::string& action, const Params& params)
{
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Print();

    std::string resp(RunAction(app, action, params));

    try
    {
        nlohmann::json val(nlohmann::json::parse(resp));

        this->debug << __func__ << ": resp:" << val.dump(4); this->debug.Print();

        if (static_cast<bool>(val["ok"])) 
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
    catch (const nlohmann::json::exception& ex) 
    {
        std::ostringstream message; message << 
            ex.what() << " ... body:" << resp;

        throw JSONErrorException(message.str()); 
    }
}

void Backend::Authenticate(const std::string& username)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Print();
//    std::cout << "Password?" << std::endl;
//    std::string password; std::getline(std::cin, password);
}

nlohmann::json Backend::GetConfig()
{
    // TODO load all 3 configs in one transaction

    nlohmann::json config;

    config["server"] = RunJsonAction("server","getconfig");
    config["accounts"] = RunJsonAction("accounts","getconfig");
    config["files"] = RunJsonAction("files","getconfig");

    return std::move(config);
}