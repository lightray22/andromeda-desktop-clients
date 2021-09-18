#include <string>
#include <iostream>
#include <utility>
#include <sstream>

#include <nlohmann/json.hpp>

#include "Backend.hpp"
#include "Utilities.hpp"

/*****************************************************/
Backend::Backend(Runner& runner) : 
    debug("Backend",this), runner(runner) { }

/*****************************************************/
Backend::~Backend()
{
    this->debug << __func__ << "()"; this->debug.Out();

    try { CloseSession(); }
    catch(const Utilities::Exception& ex) 
    { 
        this->debug << __func__ << "..." << ex.what(); this->debug.Print();
    }
}

/*****************************************************/
void Backend::Initialize()
{
    this->debug << __func__ << "()"; this->debug.Out();

    this->config.Initialize(*this);
}

/*****************************************************/
std::string Backend::RunAction(const std::string& app, const std::string& action)
{
    Params params; return std::move(RunAction(app, action, params));
}

/*****************************************************/
std::string Backend::RunAction(const std::string& app, const std::string& action, Params& params)
{
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Out();

    if (!this->sessionID.empty())
    {
        params["auth_sessionid"] = this->sessionID;
        params["auth_sessionkey"] = this->sessionKey;
    }

    return std::move(this->runner.RunAction(app, action, params));
}

/*****************************************************/
nlohmann::json Backend::GetJSON(const std::string& resp)
{
    try {
        nlohmann::json val(nlohmann::json::parse(resp));

        this->debug << __func__ << "... json:" << val.dump(4); this->debug.Out(Debug::Level::DETAILS);

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

/*****************************************************/
void Backend::Authenticate(const std::string& username, const std::string& password, const std::string& twofactor)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Out();

    CloseSession();

    Params params {{ "username", username }, { "auth_password", password }};

    if (!twofactor.empty()) params["auth_twofactor"] = twofactor;

    nlohmann::json resp(GetJSON(RunAction("accounts", "createsession", params)));

    try
    {
        this->createdSession = true;
        this->sessionID = resp["client"]["session"]["id"];
        this->sessionKey = resp["client"]["session"]["authkey"];

        this->debug << __func__ << "... sessionID:" << this->sessionID; this->debug.Out(Debug::Level::DETAILS);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Backend::AuthInteractive(const std::string& username, std::string password, std::string twofactor)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Out();

    if (password.empty())
    {
        std::cout << "Password? ";
        Utilities::SilentReadConsole(password);
    }

    try
    {
        Authenticate(username, password, twofactor);
    }
    catch (const TwoFactorRequiredException& e)
    {
        std::cout << "Two Factor? ";
        Utilities::SilentReadConsole(twofactor);

        Authenticate(username, password, twofactor);
    }
}

/*****************************************************/
void Backend::PreAuthenticate(const std::string& sessionID, const std::string& sessionKey)
{
    this->debug << __func__ << "()"; this->debug.Out();

    CloseSession();

    this->sessionID = sessionID;
    this->sessionKey = sessionKey;

    nlohmann::json resp(GetJSON(RunAction("accounts", "getaccount")));
    if (!resp.contains("id")) throw AuthenticationFailedException();
}

/*****************************************************/
void Backend::CloseSession()
{
    this->debug << __func__ << "()"; this->debug.Out();
    
    if (this->createdSession)
    {
        GetJSON(RunAction("accounts", "deleteclient"));
    }

    this->createdSession = false;
    this->sessionID.clear();
    this->sessionKey.clear();
}

/*****************************************************/
nlohmann::json Backend::GetConfig()
{
    this->debug << __func__ << "()"; this->debug.Out();

    // TODO load all configs in one transaction

    nlohmann::json config;

    config["server"] = GetJSON(RunAction("server","getconfig"));
    config["files"] = GetJSON(RunAction("files","getconfig"));

    return config;
}

/*****************************************************/
nlohmann::json Backend::GetFolder(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Out();

    Params params; if (!id.empty()) params["folder"] = id;

    return GetJSON(RunAction("files", "getfolder", params));
}
