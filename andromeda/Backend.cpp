#include <string>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "Backend.hpp"
#include "Utilities.hpp"

/*****************************************************/
Backend::Backend(Runner& runner) : 
    runner(runner), debug("Backend",this) { }

/*****************************************************/
Backend::~Backend()
{
    this->debug << __func__ << "()"; this->debug.Info();

    try { CloseSession(); }
    catch(const Utilities::Exception& ex) 
    { 
        this->debug << __func__ << "..." << ex.what(); this->debug.Error();
    }
}

/*****************************************************/
void Backend::Initialize()
{
    this->debug << __func__ << "()"; this->debug.Info();

    this->config.Initialize(*this);
}

/*****************************************************/
std::string Backend::RunAction(const std::string& app, const std::string& action)
{
    Params params; return RunAction(app, action, params);
}

/*****************************************************/
std::string Backend::RunAction(const std::string& app, const std::string& action, Params& params)
{
    this->debug << __func__ << "(app:" << app << " action:" << action << ")"; this->debug.Info();

    if (!this->sessionID.empty())
    {
        params["auth_sessionid"] = this->sessionID;
        params["auth_sessionkey"] = this->sessionKey;
    }

    return this->runner.RunAction(app, action, params);
}

/*****************************************************/
nlohmann::json Backend::GetJSON(const std::string& resp)
{
    try {
        nlohmann::json val(nlohmann::json::parse(resp));

        this->debug << __func__ << "... json:" << val.dump(4); this->debug.Details();

        if (val.at("ok").get<bool>())
            return val.at("appdata");
        else
        {
            const int code = val.at("code").get<int>();
            const auto [message, details] = Utilities::split(
                val.at("message").get<std::string>(),":");

                 if (code == 400 && message == "FILESYSTEM_MISMATCH")         throw UnsupportedException();
            else if (code == 400 && message == "STORAGE_FOLDERS_UNSUPPORTED") throw UnsupportedException();
            else if (code == 403 && message == "AUTHENTICATION_FAILED") throw AuthenticationFailedException();
            else if (code == 403 && message == "TWOFACTOR_REQUIRED")    throw TwoFactorRequiredException();

            else if (code == 403) throw DeniedException(message); 
            else if (code == 404) throw NotFoundException(message);
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
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Info();

    CloseSession();

    Params params {{ "username", username }, { "auth_password", password }};

    if (!twofactor.empty()) params["auth_twofactor"] = twofactor;

    nlohmann::json resp(GetJSON(RunAction("accounts", "createsession", params)));

    try
    {
        this->createdSession = true;
        resp.at("client").at("session").at("id").get_to(this->sessionID);
        resp.at("client").at("session").at("authkey").get_to(this->sessionKey);

        this->debug << __func__ << "... sessionID:" << this->sessionID; this->debug.Details();
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Backend::AuthInteractive(const std::string& username, std::string password, std::string twofactor)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Info();

    if (password.empty())
    {
        std::cout << "Password? ";
        Utilities::SilentReadConsole(password);
        //password = "password";
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
    this->debug << __func__ << "()"; this->debug.Info();

    CloseSession();

    this->sessionID = sessionID;
    this->sessionKey = sessionKey;

    nlohmann::json resp(GetJSON(RunAction("accounts", "getaccount")));
    if (!resp.contains("id")) throw AuthenticationFailedException();
}

/*****************************************************/
void Backend::CloseSession()
{
    this->debug << __func__ << "()"; this->debug.Info();
    
    if (this->createdSession)
    {
        GetJSON(RunAction("accounts", "deleteclient"));
    }

    this->createdSession = false;
    this->sessionID.clear();
    this->sessionKey.clear();
}

/*****************************************************/
void Backend::RequireAuthentication() const
{
    if (this->sessionID.empty())
        throw AuthRequiredException();
}

/*****************************************************/
nlohmann::json Backend::GetConfig()
{
    this->debug << __func__ << "()"; this->debug.Info();

    nlohmann::json config;

    config["server"] = GetJSON(RunAction("server","getconfig"));
    config["files"] = GetJSON(RunAction("files","getconfig"));

    return config;
}

/*****************************************************/
nlohmann::json Backend::GetFolder(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    Params params; if (!id.empty()) params["folder"] = id;

    return GetJSON(RunAction("files", "getfolder", params));
}

/*****************************************************/
nlohmann::json Backend::GetFSRoot(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    Params params {{"files","false"},{"folders","false"}}; 
    
    if (!id.empty()) params["filesystem"] = id;

    return GetJSON(RunAction("files", "getfolder", params));
}

/*****************************************************/
nlohmann::json Backend::GetFilesystem(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    Params params; if (!id.empty()) params["filesystem"] = id;

    return GetJSON(RunAction("files", "getfilesystem", params));
}

/*****************************************************/
nlohmann::json Backend::GetFilesystems()
{
    this->debug << __func__ << "()"; this->debug.Info();

    return GetJSON(RunAction("files", "getfilesystems"));
}

/*****************************************************/
nlohmann::json Backend::GetAdopted()
{
    this->debug << __func__ << "()"; this->debug.Info();

    return GetJSON(RunAction("files", "listadopted"));
}

/*****************************************************/
nlohmann::json Backend::CreateFolder(const std::string& parent, const std::string& name)
{
    this->debug << __func__ << "(parent:" << parent << " name:" << name << ")"; this->debug.Info();

    Params params {{"parent", parent},{"name", name}};

    return GetJSON(RunAction("files", "createfolder", params));
}

/*****************************************************/
void Backend::DeleteFile(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    Params params {{"file", id}};
    
    GetJSON(RunAction("files", "deletefile", params));
}

/*****************************************************/
void Backend::DeleteFolder(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    Params params {{"folder", id}}; 
    
    GetJSON(RunAction("files", "deletefolder", params));
}

/*****************************************************/
void Backend::RenameFile(const std::string& id, const std::string& name, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " name:" << name << ")"; this->debug.Info();

    Params params {{"file", id}, {"name", name}, {"overwrite", overwrite?"true":"false"}};

    GetJSON(RunAction("files", "renamefile", params));
}

/*****************************************************/
void Backend::RenameFolder(const std::string& id, const std::string& name, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " name:" << name << ")"; this->debug.Info();

    Params params {{"folder", id}, {"name", name}, {"overwrite", overwrite?"true":"false"}};

    GetJSON(RunAction("files", "renamefolder", params));
}

/*****************************************************/
void Backend::MoveFile(const std::string& id, const std::string& parent, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " parent:" << parent << ")"; this->debug.Info();

    Params params {{"file", id}, {"parent", parent}, {"overwrite", overwrite?"true":"false"}};

    GetJSON(RunAction("files", "movefile", params));
}

/*****************************************************/
void Backend::MoveFolder(const std::string& id, const std::string& parent, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " parent:" << parent << ")"; this->debug.Info();

    Params params {{"folder", id}, {"parent", parent}, {"overwrite", overwrite?"true":"false"}};

    GetJSON(RunAction("files", "movefolder", params));
}
