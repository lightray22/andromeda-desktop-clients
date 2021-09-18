#include <string>
#include <iostream>
#include <utility>
#include <sstream>

#include <nlohmann/json.hpp>

#include "Backend.hpp"
#include "Utilities.hpp"

Backend::Backend(Runner& runner) : 
    debug("Backend",this), runner(runner) { }

Backend::~Backend()
{
    try
    {
        if (this->createdSession)
            GetJSON(RunAction("accounts", "deleteclient"));
    }
    catch(const Utilities::Exception& ex) 
    { 
        this->debug << __func__ << "..." << ex.what(); this->debug.Print();
    }
}

void Backend::Initialize()
{
    this->debug << __func__ << "()"; this->debug.Print();

    this->config.Initialize(*this);
}

std::string Backend::RunAction(const std::string& app, const std::string& action)
{
    Params params; return std::move(RunAction(app, action, params));
}

std::string Backend::RunAction(const std::string& app, const std::string& action, Params& params)
{
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Print();

    if (!this->sessionID.empty())
    {
        params["auth_sessionid"] = this->sessionID;
        params["auth_sessionkey"] = this->sessionKey;
    }

    return std::move(this->runner.RunAction(app, action, params));
}

nlohmann::json Backend::GetJSON(const std::string& resp)
{
    try {
        nlohmann::json val(nlohmann::json::parse(resp));

        this->debug << __func__ << "... json:" << val.dump(4); this->debug.Print();

        if (static_cast<bool>(val["ok"])) 
            return val["appdata"];
        else
        {
            std::vector<std::string> mpair(Utilities::explode(val["message"], ":", 2));
            std::string details = (mpair.size() == 2) ? mpair[1] : "";

            std::string message = mpair[0];
            const int code = val["code"];

                 if (code == 403 && message == "AUTHENTICATION_FAILED") throw AuthenticationFailedException();
            else if (code == 403 && message == "TWOFACTOR_REQUIRED")    throw TwoFactorRequiredException();

            else if (code == 403) throw APIDeniedException(message); 
            else if (code == 404) throw APINotFoundException(message);
            else throw APIException(code, message);
        }
    }
    catch (const nlohmann::json::exception& ex) 
    {
        std::ostringstream message; message << 
            ex.what() << " ... body:" << resp;
        throw JSONErrorException(message.str()); 
    }
}

void Backend::Authenticate(const std::string& username, const std::string& password, const std::string& twofactor)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Print();

    Params params {{ "username", username }, { "auth_password", password }};

    if (!twofactor.empty()) params["auth_twofactor"] = twofactor;

    nlohmann::json resp(GetJSON(RunAction("accounts", "createsession", params)));

    try
    {
        this->createdSession = true;
        this->sessionID = resp["client"]["session"]["id"];
        this->sessionKey = resp["client"]["session"]["authkey"];

        this->debug << __func__ << "... sessionID:" << this->sessionID; this->debug.Print();
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

void Backend::AuthInteractive(const std::string& username, std::string password, std::string twofactor)
{
    if (password.empty())
    {
        std::cout << "Password?" << std::endl;
        Utilities::SilentReadConsole(password);
    }

    try
    {
        Authenticate(username, password, twofactor);
    }
    catch (const TwoFactorRequiredException& e)
    {
        std::cout << "Two Factor?" << std::endl;
        Utilities::SilentReadConsole(twofactor);

        Authenticate(username, password, twofactor);
    }
}

void Backend::PreAuthenticate(const std::string& sessionID, const std::string& sessionKey)
{
    this->sessionID = sessionID;
    this->sessionKey = sessionKey;
}

nlohmann::json Backend::GetConfig()
{
    // TODO load all configs in one transaction

    this->debug << __func__ << "()"; this->debug.Print();

    nlohmann::json config;

    config["server"] = GetJSON(RunAction("server","getconfig"));
    config["files"] = GetJSON(RunAction("files","getconfig"));

    return std::move(config);
}
