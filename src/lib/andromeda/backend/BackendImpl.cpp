
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <nlohmann/json.hpp>

#include "BackendImpl.hpp"
#include "BaseRunner.hpp"
#include "RunnerInput.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Backend {

/*****************************************************/
BackendImpl::BackendImpl(const ConfigOptions& options, BaseRunner& runner) : 
    mOptions(options), mRunner(runner), mConfig(*this), mDebug("Backend",this) { }

/*****************************************************/
BackendImpl::~BackendImpl()
{
    mDebug << __func__ << "()"; mDebug.Info();

    try { CloseSession(); }
    catch(const BaseException& ex) 
    { 
        mDebug << __func__ << "... " << ex.what(); mDebug.Error();
    }
}

/*****************************************************/
void BackendImpl::Initialize()
{
    mDebug << __func__ << "()"; mDebug.Info();

    mConfig.Initialize();
}

/*****************************************************/
bool BackendImpl::isReadOnly() const
{
    return mOptions.readOnly || mConfig.isReadOnly();
}

/*****************************************************/
std::string BackendImpl::GetName(bool human) const
{
    std::string hostname { mRunner.GetHostname() };

    if (mUsername.empty()) return hostname;
    else return mUsername + (human ? " on " : "_") + hostname;
}

/*****************************************************/
std::string BackendImpl::RunAction(RunnerInput& input)
{
    mReqCount++;

    if (mDebug)
    {
        mDebug << __func__ << "() " << mReqCount
            << " app:" << input.app << " action:" << input.action;

        for (const auto& [key,val] : input.params)
            mDebug << " " << key << ":" << val;

        for (const auto& [key,file] : input.files)
            mDebug << " " << key << ":" << file.name << ":" << file.data.size();

        for (const auto& [key,file] : input.sfiles)
            mDebug << " " << key << ":" << file.name;

        mDebug.Backend();
    }

    if (!mSessionID.empty())
    {
        input.params["auth_sessionid"] = mSessionID;
        input.params["auth_sessionkey"] = mSessionKey;
    }
    else if (!mUsername.empty())
    {
        input.params["auth_sudouser"] = mUsername;
    }

    return mRunner.RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetJSON(const std::string& resp)
{
    try {
        nlohmann::json val(nlohmann::json::parse(resp));

        mDebug << __func__ << "... json:" << val.dump(4); mDebug.Info();

        if (val.at("ok").get<bool>())
            return val.at("appdata");
        else
        {
            const int code { val.at("code").get<int>() };
            const auto [message, details] { Utilities::split(
                val.at("message").get<std::string>(),":") };
            
            mDebug << __func__ << "... message:" << message; mDebug.Backend();

                 if (code == 400 && message == "FILESYSTEM_MISMATCH")         throw UnsupportedException();
            else if (code == 400 && message == "STORAGE_FOLDERS_UNSUPPORTED") throw UnsupportedException();
            else if (code == 400 && message == "ACCOUNT_CRYPTO_NOT_UNLOCKED") throw DeniedException(message);
            else if (code == 403 && message == "AUTHENTICATION_FAILED") throw AuthenticationFailedException();
            else if (code == 403 && message == "TWOFACTOR_REQUIRED")    throw TwoFactorRequiredException();
            else if (code == 403 && message == "READ_ONLY_DATABASE")   throw ReadOnlyException("Database");
            else if (code == 403 && message == "READ_ONLY_FILESYSTEM") throw ReadOnlyException("Filesystem");

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
void BackendImpl::Authenticate(const std::string& username, const std::string& password, const std::string& twofactor)
{
    mDebug << __func__ << "(username:" << username << ")"; mDebug.Info();

    CloseSession();

    RunnerInput input { "accounts", "createsession", {{ "username", username }, { "auth_password", password }}};

    if (!twofactor.empty()) input.params["auth_twofactor"] = twofactor;

    nlohmann::json resp(GetJSON(RunAction(input)));

    try
    {
        mCreatedSession = true;

        resp.at("account").at("id").get_to(mAccountID);
        resp.at("client").at("session").at("id").get_to(mSessionID);
        resp.at("client").at("session").at("authkey").get_to(mSessionKey);

        mDebug << __func__ << "... sessionID:" << mSessionID; mDebug.Info();
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mUsername = username;
    mConfig.LoadAccountLimits(*this);
}

/*****************************************************/
void BackendImpl::AuthInteractive(const std::string& username, std::string password, bool forceSession)
{
    mDebug << __func__ << "(username:" << username << ")"; mDebug.Info();

    CloseSession();

    if (mRunner.RequiresSession() || forceSession || !password.empty())
    {
        if (password.empty())
        {
            std::cout << "Password? ";
            Utilities::SilentReadConsole(password);
        }

        try
        {
            Authenticate(username, password);
        }
        catch (const TwoFactorRequiredException&)
        {
            std::string twofactor; std::cout << "Two Factor? ";
            Utilities::SilentReadConsole(twofactor);

            Authenticate(username, password, twofactor);
        }
    }
    else 
    {
        mUsername = username;
        mConfig.LoadAccountLimits(*this);
    }
}

/*****************************************************/
void BackendImpl::PreAuthenticate(const std::string& sessionID, const std::string& sessionKey)
{
    mDebug << __func__ << "()"; mDebug.Info();

    CloseSession();

    mSessionID = sessionID;
    mSessionKey = sessionKey;

    RunnerInput input {"accounts", "getaccount"};

    nlohmann::json resp(GetJSON(RunAction(input)));

    try
    {
        resp.at("id").get_to(mAccountID);
        resp.at("username").get_to(mUsername);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void BackendImpl::CloseSession()
{
    mDebug << __func__ << "()"; mDebug.Info();
    
    if (mCreatedSession)
    {
        RunnerInput input {"accounts", "deleteclient"};

        GetJSON(RunAction(input));
    }

    mCreatedSession = false;
    mUsername.clear();
    mSessionID.clear();
    mSessionKey.clear();
}

/*****************************************************/
void BackendImpl::RequireAuthentication() const
{
    if (mRunner.RequiresSession())
    {
        if (mSessionID.empty())
            throw AuthRequiredException();
    }
    else
    {
        if (mSessionID.empty() && mUsername.empty())
            throw AuthRequiredException();
    }
}

/*****************************************************/
bool BackendImpl::isMemory() const
{
    return mOptions.cacheType == ConfigOptions::CacheType::MEMORY;
}

/*****************************************************/
nlohmann::json BackendImpl::GetConfigJ()
{
    mDebug << __func__ << "()"; mDebug.Info();

    nlohmann::json configJ;

    {
        RunnerInput input {"core", "getconfig"};
        configJ["core"] = GetJSON(RunAction(input));
    }

    {
        RunnerInput input {"files", "getconfig"};
        configJ["files"] = GetJSON(RunAction(input));
    }

    return configJ;
}

/*****************************************************/
nlohmann::json BackendImpl::GetAccountLimits()
{
    if (mAccountID.empty())
        return nullptr;

    RunnerInput input {"files", "getlimits", {{"account", mAccountID}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::GetFolder(const std::string& id)
{
    mDebug << __func__ << "(id:" << id << ")"; mDebug.Info();

    if (isMemory() && id.empty())
    {
        nlohmann::json retval;

        retval["files"] = std::map<std::string,int>();
        retval["folders"] = std::map<std::string,int>();

        return retval;
    }

    RunnerInput input {"files", "getfolder"}; 
    
    if (!id.empty()) input.params["folder"] = id;

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::GetFSRoot(const std::string& id)
{
    mDebug << __func__ << "(id:" << id << ")"; mDebug.Info();

    RunnerInput input {"files", "getfolder"}; 
    
    if (!id.empty()) input.params["filesystem"] = id;

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesystem(const std::string& id)
{
    mDebug << __func__ << "(id:" << id << ")"; mDebug.Info();

    if (isMemory() && id.empty()) return nullptr;

    RunnerInput input {"files", "getfilesystem"};

    if (!id.empty()) input.params["filesystem"] = id;

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::GetFSLimits(const std::string& id)
{
    mDebug << __func__ << "(id:" << id << ")"; mDebug.Info();

    if (isMemory() && id.empty()) return nullptr;

    RunnerInput input {"files", "getlimits", {{"filesystem", id}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesystems()
{
    mDebug << __func__ << "()"; mDebug.Info();

    RunnerInput input {"files", "getfilesystems"};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::GetAdopted()
{
    mDebug << __func__ << "()"; mDebug.Info();

    RunnerInput input {"files", "listadopted"};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::CreateFile(const std::string& parent, const std::string& name)
{
    mDebug << __func__ << "(parent:" << parent << " name:" << name << ")"; mDebug.Info();

    if (isMemory())
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"size", 0}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};
        
        return retval;
    }

    RunnerInput input {"files", "upload", {{"parent", parent}, {"name", name}}, {{"file", {name, ""}}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::CreateFolder(const std::string& parent, const std::string& name)
{
    mDebug << __func__ << "(parent:" << parent << " name:" << name << ")"; mDebug.Info();

    if (isMemory())
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};

        retval["files"] = std::map<std::string,int>(); 
        retval["folders"] = std::map<std::string,int>();

        return retval;
    }

    RunnerInput input {"files", "createfolder", {{"parent", parent},{"name", name}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
void BackendImpl::DeleteFile(const std::string& id)
{
    mDebug << __func__ << "(id:" << id << ")"; mDebug.Info();

    if (isMemory()) return;

    RunnerInput input {"files", "deletefile", {{"file", id}}};
    
    try { GetJSON(RunAction(input)); }
    catch (const NotFoundException&) { }
}

/*****************************************************/
void BackendImpl::DeleteFolder(const std::string& id)
{
    mDebug << __func__ << "(id:" << id << ")"; mDebug.Info();

    if (isMemory()) return;

    RunnerInput input {"files", "deletefolder", {{"folder", id}}}; 
    
    try { GetJSON(RunAction(input)); }
    catch (const NotFoundException&) { }
}

/*****************************************************/
nlohmann::json BackendImpl::RenameFile(const std::string& id, const std::string& name, bool overwrite)
{
    mDebug << __func__ << "(id:" << id << " name:" << name << ")"; mDebug.Info();

    if (isMemory()) return nullptr;

    RunnerInput input {"files", "renamefile", {{"file", id}, {"name", name}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::RenameFolder(const std::string& id, const std::string& name, bool overwrite)
{
    mDebug << __func__ << "(id:" << id << " name:" << name << ")"; mDebug.Info();

    if (isMemory()) return nullptr;

    RunnerInput input {"files", "renamefolder", {{"folder", id}, {"name", name}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::MoveFile(const std::string& id, const std::string& parent, bool overwrite)
{
    mDebug << __func__ << "(id:" << id << " parent:" << parent << ")"; mDebug.Info();

    if (isMemory()) return nullptr;

    RunnerInput input {"files", "movefile", {{"file", id}, {"parent", parent}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::MoveFolder(const std::string& id, const std::string& parent, bool overwrite)
{
    mDebug << __func__ << "(id:" << id << " parent:" << parent << ")"; mDebug.Info();

    if (isMemory()) return nullptr;

    RunnerInput input {"files", "movefolder", {{"folder", id}, {"parent", parent}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
std::string BackendImpl::ReadFile(const std::string& id, const uint64_t offset, const size_t length)
{
    std::string fstart(std::to_string(offset));
    std::string flast(std::to_string(offset+length-1));

    mDebug << __func__ << "(id:" << id << " fstart:" << fstart << " flast:" << flast; mDebug.Info();

    if (isMemory()) return std::string(length,'\0');

    RunnerInput input {"files", "download", {{"file", id}, {"fstart", fstart}, {"flast", flast}}};

    std::string data(RunAction(input));

    if (data.size() != length) throw ReadSizeException(length, data.size());

    return data;
}

/*****************************************************/
nlohmann::json BackendImpl::WriteFile(const std::string& id, const uint64_t offset, const std::string& data)
{
    mDebug << __func__ << "(id:" << id << " offset:" << offset << " size:" << data.size(); mDebug.Info();

    if (isMemory()) return nullptr;

    RunnerInput input {"files", "writefile", {{"file", id}, {"offset", std::to_string(offset)}}, {{"data", {"data", data}}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json BackendImpl::TruncateFile(const std::string& id, const uint64_t size)
{
    mDebug << __func__ << "(id:" << id << " size:" << size << ")"; mDebug.Info();

    if (isMemory()) return nullptr;

    RunnerInput input {"files", "ftruncate", {{"file", id}, {"size", std::to_string(size)}}};

    return GetJSON(RunAction(input));
}

} // namespace Backend
} // namespace Andromeda
