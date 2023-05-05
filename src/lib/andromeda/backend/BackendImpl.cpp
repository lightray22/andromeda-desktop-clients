
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <nlohmann/json.hpp>

#include "BackendImpl.hpp"
#include "HTTPRunner.hpp"
#include "RunnerInput.hpp"
#include "RunnerPool.hpp"
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace Backend {

#define BOOLSTR(x) (x ? "true" : "false")

std::atomic<uint64_t> BackendImpl::sReqCount { 0 };

/*****************************************************/
BackendImpl::BackendImpl(const ConfigOptions& options, RunnerPool& runners) : 
    mOptions(options), mRunners(runners),
    mDebug("Backend",this) , mConfig(*this)
{ 
    MDBG_INFO("()");
}

/*****************************************************/
BackendImpl::~BackendImpl()
{
    MDBG_INFO("()");

    try { CloseSession(); }
    catch(const BaseException& ex) 
    { 
        MDBG_ERROR("... " << ex.what());
    }
}

/*****************************************************/
bool BackendImpl::isReadOnly() const
{
    return mOptions.readOnly || mConfig.isReadOnly();
}

/*****************************************************/
std::string BackendImpl::GetName(bool human) const
{
    std::string hostname { mRunners.GetFirst().GetHostname() };

    if (mUsername.empty()) return hostname;
    
    if (human) return mUsername+" on "+hostname;
    else return hostname+"_"+mUsername;
}

/*****************************************************/
void BackendImpl::PrintInput(const RunnerInput& input, std::ostream& str, const std::string& myfname)
{
    str << ++sReqCount << " " << myfname << "()"
        << " app:" << input.app << " action:" << input.action;
        
    for (const auto& [key,val] : input.params)
        str << " " << key << ":" << val;
}

/*****************************************************/
void BackendImpl::PrintInput(const RunnerInput_FilesIn& input, std::ostream& str, const std::string& myfname)
{
    PrintInput(static_cast<const RunnerInput&>(input), str, myfname);

    for (const auto& [key,file] : input.files)
        str << " " << key << ":" << file.name << ":" << file.data.size();
}

/*****************************************************/
void BackendImpl::PrintInput(const RunnerInput_StreamIn& input, std::ostream& str, const std::string& myfname)
{
    PrintInput(static_cast<const RunnerInput_FilesIn&>(input), str, myfname);

    for (const auto& [key,fstr] : input.fstreams)
        str << " " << key << ":" << fstr.name << ":" << "(stream)";
}

/** Syntactic sugar to print and input object and current function name to debug */
#define MDBG_BACKEND(input) { static const std::string myfname(__func__); \
    mDebug.Backend([&](std::ostream& str){ PrintInput(input, str, myfname); }); }

/*****************************************************/
template <class InputT>
InputT& BackendImpl::FinalizeInput(InputT& input)
{
    if (!mSessionID.empty())
    {
        input.params["auth_sessionid"] = mSessionID;
        input.params["auth_sessionkey"] = mSessionKey;
    }
    else if (!mUsername.empty())
    {
        input.params["auth_sudouser"] = mUsername;
    }

    return input;
}

/*****************************************************/
nlohmann::json BackendImpl::GetJSON(const std::string& resp)
{
    try {
        nlohmann::json val(nlohmann::json::parse(resp));

        MDBG_INFO("... json:" << val.dump(4));

        if (val.at("ok").get<bool>())
            return val.at("appdata");
        else
        {
            const int code { val.at("code").get<int>() };
            const auto [message, details] { Utilities::split(
                val.at("message").get<std::string>(),":") };

            const std::string fname(__func__);
            mDebug.Backend([fname,message=&message](std::ostream& str){ 
                str << fname << "... message:" << *message; });

                 if (code == 400 && message == "FILESYSTEM_MISMATCH")         throw UnsupportedException();
            else if (code == 400 && message == "STORAGE_FOLDERS_UNSUPPORTED") throw UnsupportedException();
            else if (code == 400 && message == "ACCOUNT_CRYPTO_NOT_UNLOCKED") throw DeniedException(message); // TODO better exception? - should not happen if Authenticated? maybe for bad shares
            else if (code == 400 && message == "INPUT_FILE_MISSING")          throw HTTPRunner::InputSizeException();
            
            else if (code == 403 && message == "AUTHENTICATION_FAILED") throw AuthenticationFailedException();
            else if (code == 403 && message == "TWOFACTOR_REQUIRED")    throw TwoFactorRequiredException();
            else if (code == 403 && message == "READ_ONLY_DATABASE")   throw ReadOnlyFSException("Database");
            else if (code == 403 && message == "READ_ONLY_FILESYSTEM") throw ReadOnlyFSException("Filesystem");

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
template <class InputT>
nlohmann::json BackendImpl::RunAction(InputT& input)
{
    return GetJSON(mRunners.GetRunner()->RunAction(FinalizeInput(input)));
}

/*****************************************************/
void BackendImpl::Authenticate(const std::string& username, const std::string& password, const std::string& twofactor)
{
    MDBG_INFO("(username:" << username << ")");

    CloseSession();

    RunnerInput input { "accounts", "createsession", {{ "username", username }, { "auth_password", password }}};

    if (!twofactor.empty()) 
        input.params["auth_twofactor"] = twofactor; 
    MDBG_BACKEND(input);

    nlohmann::json resp(RunAction(input));

    try
    {
        mCreatedSession = true;

        resp.at("account").at("id").get_to(mAccountID);
        resp.at("client").at("session").at("id").get_to(mSessionID);
        resp.at("client").at("session").at("authkey").get_to(mSessionKey);

        MDBG_INFO("... sessionID:" << mSessionID);
    }
    catch (const nlohmann::json::exception& ex) {
        throw JSONErrorException(ex.what()); }

    mUsername = username;
    mConfig.LoadAccountLimits(*this);
}

/*****************************************************/
void BackendImpl::AuthInteractive(const std::string& username, std::string password, bool forceSession)
{
    MDBG_INFO("(username:" << username << ")");

    CloseSession();

    if (mRunners.GetFirst().RequiresSession() || forceSession || !password.empty())
    {
        if (password.empty())
        {
            if (mOptions.quiet) throw AuthenticationFailedException();

            std::cout << "Password? ";
            Utilities::SilentReadConsole(password);
        }

        try
        {
            Authenticate(username, password);
        }
        catch (const TwoFactorRequiredException&)
        {
            if (mOptions.quiet) throw; // rethrow

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
    MDBG_INFO("()");

    CloseSession();

    mSessionID = sessionID;
    mSessionKey = sessionKey;

    RunnerInput input {"accounts", "getaccount"}; MDBG_BACKEND(input);

    nlohmann::json resp(RunAction(input));

    try
    {
        resp.at("id").get_to(mAccountID);
        resp.at("username").get_to(mUsername);
    }
    catch (const nlohmann::json::exception& ex) {
        throw JSONErrorException(ex.what()); }
}

/*****************************************************/
void BackendImpl::CloseSession()
{
    MDBG_INFO("()");
    
    if (mCreatedSession)
    {
        RunnerInput input {"accounts", "deleteclient"}; MDBG_BACKEND(input);

        RunAction(input);
    }

    mCreatedSession = false;
    mUsername.clear();
    mSessionID.clear();
    mSessionKey.clear();
}

/*****************************************************/
void BackendImpl::RequireAuthentication() const
{
    if (mRunners.GetFirst().RequiresSession())
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
    MDBG_INFO("()");

    nlohmann::json configJ;

    {
        RunnerInput input {"core", "getconfig"}; MDBG_BACKEND(input);
        configJ["core"] = RunAction(input);
    }

    {
        RunnerInput input {"files", "getconfig"}; MDBG_BACKEND(input);
        configJ["files"] = RunAction(input);
    }

    return configJ;
}

/*****************************************************/
nlohmann::json BackendImpl::GetAccountLimits()
{
    if (mAccountID.empty())
        return nullptr;

    RunnerInput input {"files", "getlimits", {{"account", mAccountID}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFolder(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory()) /** debug only */
    {
        nlohmann::json retval;
        retval["id"] = id;
        retval["files"] = std::map<std::string,int>();
        retval["folders"] = std::map<std::string,int>();
        return retval;
    }

    RunnerInput input {"files", "getfolder", {{"folder", id}}}; MDBG_BACKEND(input);
    
    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFSRoot(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory()) /** debug only */
    {
        nlohmann::json retval;
        retval["id"] = id;
        retval["files"] = std::map<std::string,int>();
        retval["folders"] = std::map<std::string,int>();
        return retval;
    }

    RunnerInput input {"files", "getfolder", {{"filesystem", id}}}; MDBG_BACKEND(input);
    
    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesystem(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory() && id.empty()) return nullptr;

    RunnerInput input {"files", "getfilesystem", {{"filesystem", id}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFSLimits(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory() && id.empty()) return nullptr;

    RunnerInput input {"files", "getlimits", {{"filesystem", id}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesystems()
{
    MDBG_INFO("()");

    RunnerInput input {"files", "getfilesystems"}; MDBG_BACKEND(input);

    return RunAction(input); 
}

/*****************************************************/
nlohmann::json BackendImpl::GetAdopted()
{
    MDBG_INFO("()");

    RunnerInput input {"files", "listadopted"}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::CreateFile(const std::string& parent, const std::string& name, bool overwrite)
{
    MDBG_INFO("(parent:" << parent << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"size", 0}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};
        
        return retval;
    }

    RunnerInput_FilesIn input {{"files", "upload", {{"parent", parent}, {"name", name}, 
        {"overwrite", BOOLSTR(overwrite)}}}, {{"file", {name, ""}}}}; MDBG_BACKEND(input);
        
    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::CreateFolder(const std::string& parent, const std::string& name)
{
    MDBG_INFO("(parent:" << parent << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};

        retval["files"] = std::map<std::string,int>(); 
        retval["folders"] = std::map<std::string,int>();

        return retval;
    }

    RunnerInput input {"files", "createfolder", {{"parent", parent}, {"name", name}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
void BackendImpl::DeleteFile(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return;

    RunnerInput input {"files", "deletefile", {{"file", id}}}; MDBG_BACKEND(input);
    
    try { RunAction(input); }
    catch (const NotFoundException& e) {
        MDBG_INFO("... backend:" << e.what()); }
}

/*****************************************************/
void BackendImpl::DeleteFolder(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return;

    RunnerInput input {"files", "deletefolder", {{"folder", id}}}; MDBG_BACKEND(input);
    
    try { RunAction(input); }
    catch (const NotFoundException& e) {
        MDBG_INFO("... backend:" << e.what()); }
}

/*****************************************************/
nlohmann::json BackendImpl::RenameFile(const std::string& id, const std::string& name, bool overwrite)
{
    MDBG_INFO("(id:" << id << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return nullptr;

    RunnerInput input {"files", "renamefile", {{"file", id}, {"name", name}, {"overwrite", BOOLSTR(overwrite)}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::RenameFolder(const std::string& id, const std::string& name, bool overwrite)
{
    MDBG_INFO("(id:" << id << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return nullptr;

    RunnerInput input {"files", "renamefolder", {{"folder", id}, {"name", name}, {"overwrite", BOOLSTR(overwrite)}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::MoveFile(const std::string& id, const std::string& parent, bool overwrite)
{
    MDBG_INFO("(id:" << id << " parent:" << parent << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return nullptr;

    RunnerInput input {"files", "movefile", {{"file", id}, {"parent", parent}, {"overwrite", BOOLSTR(overwrite)}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
nlohmann::json BackendImpl::MoveFolder(const std::string& id, const std::string& parent, bool overwrite)
{
    MDBG_INFO("(id:" << id << " parent:" << parent << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return nullptr;

    RunnerInput input {"files", "movefolder", {{"folder", id}, {"parent", parent}, {"overwrite", BOOLSTR(overwrite)}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

/*****************************************************/
std::string BackendImpl::ReadFile(const std::string& id, const uint64_t offset, const size_t length)
{
    if (!length) { MDBG_ERROR("() ERROR 0 length"); assert(false); return ""; }

    std::string fstart(std::to_string(offset));
    std::string flast(std::to_string(offset+length-1));

    MDBG_INFO("(id:" << id << " fstart:" << fstart << " flast:" << flast << ")");

    if (isMemory()) /** debug only */ return std::string(length,'\0');

    RunnerInput input {"files", "download", {{"file", id}, {"fstart", fstart}, {"flast", flast}}}; MDBG_BACKEND(input);

    std::string data { mRunners.GetRunner()->RunAction(FinalizeInput(input)) }; // not JSON
    if (data.size() != length) throw ReadSizeException(length, data.size());

    return data;
}

/*****************************************************/
void BackendImpl::ReadFile(const std::string& id, const uint64_t offset, const size_t length, const ReadFunc& userFunc)
{
    if (!length) { MDBG_ERROR("() ERROR 0 length"); assert(false); return; }

    std::string fstart(std::to_string(offset));
    std::string flast(std::to_string(offset+length-1));

    MDBG_INFO("(id:" << id << " fstart:" << fstart << " flast:" << flast << ")");

    if (isMemory()) /** debug only */ { userFunc(0, std::string(length,'\0').data(), length); return; }

    size_t read = 0; RunnerInput_StreamOut input {{"files", "download", {{"file", id}, {"fstart", fstart}, {"flast", flast}}}, 
        [&](const size_t soffset, const char* buf, const size_t buflen)->void
    {
        userFunc(soffset, buf, buflen); read += buflen;
    }}; MDBG_BACKEND(input);

    mRunners.GetRunner()->RunAction(FinalizeInput(input)); // Not JSON

    if (read != length) throw ReadSizeException(length, read);
}

// if we get a 413 this small the server must be bugged
constexpr size_t UPLOAD_MINSIZE { 4*1024 };
// multiply the failed by this to get the next attempt size
constexpr size_t ADJUST_ATTEMPT(size_t maxSize){ return maxSize/2; }

/*****************************************************/
nlohmann::json BackendImpl::WriteFile(const std::string& id, const uint64_t offset, const std::string& data)
{
    if (data.empty()) { MDBG_ERROR("() ERROR no data"); assert(false); }

    MDBG_INFO("(id:" << id << " offset:" << offset << " size:" << data.size() << ")");

    return WriteFile(id, offset, RunnerInput_StreamIn::FromString(data));
}

/*****************************************************/
nlohmann::json BackendImpl::WriteFile(const std::string& id, const uint64_t offset, const WriteFunc& userFunc)
{
    MDBG_INFO("(id:" << id << " offset:" << offset << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return nullptr;

    return SendFile(userFunc, id, offset, nullptr, false);
}

/*****************************************************/
nlohmann::json BackendImpl::UploadFile(const std::string& parent, const std::string& name, const std::string& data, bool oneshot, bool overwrite)
{
    if (data.empty()) { MDBG_ERROR("() ERROR no data"); assert(false); }

    MDBG_INFO("(parent:" << parent << " name:" << name << " size:" << data.size() << ")");

    return UploadFile(parent, name, RunnerInput_StreamIn::FromString(data), oneshot, overwrite);
}

/*****************************************************/
nlohmann::json BackendImpl::UploadFile(const std::string& parent, const std::string& name, const WriteFunc& userFunc, bool oneshot, bool overwrite)
{
    MDBG_INFO("(parent:" << parent << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, 
            {"size", RunnerInput_StreamIn::StreamSize(userFunc)}, {"filesystem", ""}};
        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};
        return retval;
    }

    return SendFile(userFunc, "", 0, [&](const WriteFunc& writeFunc)->RunnerInput_StreamIn
    {
        return {{"files", "upload", {{"parent", parent}, {"name", name}, 
            {"overwrite", BOOLSTR(overwrite)}}}, {{"file", {name, writeFunc}}}};
    }, oneshot);
}

/*****************************************************/
nlohmann::json BackendImpl::SendFile(const WriteFunc& userFunc, std::string id, const uint64_t offset, const UploadInput& getUpload, bool oneshot)
{
    nlohmann::json retval;    // last json response to return
    size_t byte { 0 };        // starting stream offset to read
    bool streamCont { true }; // true if the userFunc has more
    
    while (streamCont) // retry or chunk by MaxBytes
    {
        const size_t maxSize { mConfig.GetUploadMaxBytes() };
        MDBG_INFO("... byte:" << byte << " maxSize:" << maxSize);

        size_t streamSize { 0 }; // total bytes read during stream
        const WriteFunc& writeFunc { [&](const size_t soffset, char* const buf, const size_t buflen, size_t& sread)->bool
        {
            if (maxSize && soffset >= maxSize)
            {
                if (oneshot) throw WriteSizeException();
                else return false; // end of chunk
            }

            const size_t strSize { maxSize ? std::min(buflen,maxSize) : buflen };
            streamCont = userFunc(soffset+byte, buf, strSize, sread);
            streamSize += sread; return streamCont;
        }};

        RunnerInput_StreamIn input;
        if (!byte && getUpload) // upload file
            input = getUpload(writeFunc);
        else // write file
        {
            input = {{"files", "writefile", {{"file", id}, 
                {"offset", std::to_string(offset+byte)}}}, {{"data", {"data", writeFunc}}}};
        }
        MDBG_BACKEND(input);

        try
        {
            retval = RunAction(input);
            retval.at("id").get_to(id);
            byte += streamSize; // next chunk
        }
        catch (const HTTPRunner::InputSizeException& e)
        {
            MDBG_INFO("... caught InputSizeException! streamSize:" << streamSize);

            if (maxSize && maxSize < UPLOAD_MINSIZE) {
                MDBG_ERROR("... below UPLOAD_MINSIZE!"); throw; } // rethrow
            mConfig.SetUploadMaxBytes(ADJUST_ATTEMPT(streamSize));

            if (oneshot) throw WriteSizeException();
            else streamCont = true; // need to retry chunk
        }
        catch (const nlohmann::json::exception& ex) {
            throw JSONErrorException(ex.what()); }
    }
    return retval;
}

/*****************************************************/
nlohmann::json BackendImpl::TruncateFile(const std::string& id, const uint64_t size)
{
    MDBG_INFO("(id:" << id << " size:" << size << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) /** debug only */ return nullptr;

    RunnerInput input {"files", "ftruncate", {{"file", id}, {"size", std::to_string(size)}}}; MDBG_BACKEND(input);

    return RunAction(input);
}

} // namespace Backend
} // namespace Andromeda
