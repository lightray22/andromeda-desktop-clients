
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "nlohmann/json.hpp"

#include "BackendImpl.hpp"
#include "HTTPRunner.hpp"
#include "RunnerInput.hpp"
#include "RunnerPool.hpp"
#include "SessionStore.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/Crypto.hpp"
#include "andromeda/PlatformUtil.hpp"
#include "andromeda/StringUtil.hpp"
#include "andromeda/filesystem/filedata/CacheManager.hpp"
#include "andromeda/filesystem/filedata/CachingAllocator.hpp"
using Andromeda::Filesystem::Filedata::CachingAllocator;

namespace Andromeda {
namespace Backend {

std::atomic<uint64_t> BackendImpl::sReqNext { 1 };

/*****************************************************/
BackendImpl::BackendImpl(const ConfigOptions& options, RunnerPool& runners) : 
    mOptions(options), mRunners(runners),
    mDebug("Backend",this) , mConfig(*this)
    // loading mConfig now has the nice side effect of making sure any potential
    // HTTP->HTTPS redirect is out of the way before trying other actions!
{ 
    MDBG_INFO("()");
}

/*****************************************************/
BackendImpl::~BackendImpl()
{
    MDBG_INFO("()");

    try { CloseSession(); }
    catch (const BackendException& ex) 
    { 
        MDBG_ERROR("... " << ex.what());
    }
}

/*****************************************************/
CachingAllocator& BackendImpl::GetPageAllocator()
{
    if (mCacheMgr) 
        return mCacheMgr->GetPageAllocator();

    if (!mPageAllocator)
        mPageAllocator = std::make_unique<CachingAllocator>(0);
    return *mPageAllocator;
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
void BackendImpl::PrintInput(const RunnerInput& input, std::ostream& str, const std::string& myfname, const uint64_t reqCount)
{
    str << reqCount << " " << myfname << "()"
        << " app:" << input.app << " action:" << input.action;
    
    for (const auto& [key,val] : input.plainParams)
        str << " " << key << ":" << val;
    
    for (const auto& [key,val] : input.dataParams)
        str << " (" << key << ":" << val << ")";
}

/*****************************************************/
void BackendImpl::PrintInput(const RunnerInput_FilesIn& input, std::ostream& str, const std::string& myfname, const uint64_t reqCount)
{
    PrintInput(static_cast<const RunnerInput&>(input), str, myfname, reqCount);

    for (const auto& [key,file] : input.files)
        str << " " << key << ":" << file.name << ":" << file.data.size();
}

/*****************************************************/
void BackendImpl::PrintInput(const RunnerInput_StreamIn& input, std::ostream& str, const std::string& myfname, const uint64_t reqCount)
{
    PrintInput(static_cast<const RunnerInput_FilesIn&>(input), str, myfname, reqCount);

    for (const auto& [key,fstr] : input.fstreams)
        str << " " << key << ":" << fstr.name << ":" << "(stream)";
}

/** Print an input object and current function name to debug */
#define MDBG_BACKEND(input) { \
    const uint64_t reqCount { sReqNext.fetch_add(1) }; const char* const myfname(__func__); \
    mDebug.Backend([&](std::ostream& str){ PrintInput(input, str, myfname, reqCount); }); }
    // note std::atomic only allows postfix not prefix operators!

/*****************************************************/
template <class InputT>
InputT& BackendImpl::FinalizeInput(InputT& input)
{
    if (!mSessionID.empty())
    {
        input.dataParams["auth_sessionid"] = mSessionID;
        input.dataParams["auth_sessionkey"] = mSessionKey;
    }
    else if (!mUsername.empty())
    {
        input.plainParams["auth_sudouser"] = mUsername;
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
            const StringUtil::StringPair mpair { StringUtil::split(
                val.at("message").get<std::string>(),":") };
            const std::string& message { mpair.first };

            const std::string fname(__func__); // cannot be static
            mDebug.Backend([fname,message=&message](std::ostream& str){ 
                str << fname << "... message:" << *message; });

            enum {
                HTTP_ERROR = 400,
                HTTP_DENIED = 403,
                HTTP_NOT_FOUND = 404
            };

            if      (code == HTTP_ERROR && message == "FILESYSTEM_MISMATCH")         throw UnsupportedException();
            else if (code == HTTP_ERROR && message == "STORAGE_FOLDERS_UNSUPPORTED") throw UnsupportedException();
                // TODO better exception? - should not happen if Authenticated? maybe for bad shares
            else if (code == HTTP_ERROR && message == "ACCOUNT_CRYPTO_NOT_UNLOCKED") throw DeniedException(message);
            else if (code == HTTP_ERROR && message == "INPUT_FILE_MISSING")          throw HTTPRunner::InputSizeException(); // PHP silently discards too-large files

            else if (code == HTTP_DENIED && message == "AUTHENTICATION_FAILED") throw AuthenticationFailedException();
            else if (code == HTTP_DENIED && message == "TWOFACTOR_REQUIRED")    throw TwoFactorRequiredException();
            else if (code == HTTP_DENIED && message == "READ_ONLY_DATABASE")    throw ReadOnlyFSException("Database");
            else if (code == HTTP_DENIED && message == "READ_ONLY_FILESYSTEM")  throw ReadOnlyFSException("Filesystem");

            else if (code == HTTP_DENIED) throw DeniedException(message); 
            else if (code == HTTP_NOT_FOUND) throw NotFoundException(message);
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
std::string BackendImpl::RunAction_ReadStr(RunnerInput& input)
{
    return mRunners.GetRunner()->RunAction_Read(FinalizeInput(input));
}

/*****************************************************/
nlohmann::json BackendImpl::RunAction_Read(RunnerInput& input)
{
    return GetJSON(mRunners.GetRunner()->RunAction_Read(FinalizeInput(input)));
}

/*****************************************************/
nlohmann::json BackendImpl::RunAction_Write(RunnerInput& input)
{
    return GetJSON(mRunners.GetRunner()->RunAction_Write(FinalizeInput(input)));
}

/*****************************************************/
nlohmann::json BackendImpl::RunAction_FilesIn(RunnerInput_FilesIn& input)
{
    return GetJSON(mRunners.GetRunner()->RunAction_FilesIn(FinalizeInput(input)));
}

/*****************************************************/
nlohmann::json BackendImpl::RunAction_StreamIn(RunnerInput_StreamIn& input)
{
    return GetJSON(mRunners.GetRunner()->RunAction_StreamIn(FinalizeInput(input)));
}

/*****************************************************/
void BackendImpl::RunAction_StreamOut(RunnerInput_StreamOut& input)
{
    mRunners.GetRunner()->RunAction_StreamOut(FinalizeInput(input));
}

/*****************************************************/
void BackendImpl::Authenticate(const std::string& username, const std::string& password, const std::string& twofactor)
{
    // TODO should be using SecureBuffer for passwords/sessionKey, etc... this is cheating
    const SecureBuffer passwordBuf(password.data(), password.size());

    MDBG_INFO("(username:" << username << ")");

    CloseSession();

    const size_t keySize { Crypto::SecretKeyLength() };
    const std::string password_fullkey_salt(Crypto::SaltLength(),'\0'); // no salt here
    const SecureBuffer password_fullkey { Crypto::DeriveKey(passwordBuf,password_fullkey_salt,keySize*2) };

    const SecureBuffer password_authkey { password_fullkey.substr(0,keySize) };
    const SecureBuffer password_cryptkey { password_fullkey.substr(keySize,keySize) };

    MDBG_INFO("... password_authkey:"); mDebug.Info(mDebug.DumpBytes(password_authkey.data(), password_authkey.size()));
    MDBG_INFO("... password_cryptkey:"); mDebug.Info(mDebug.DumpBytes(password_cryptkey.data(), password_cryptkey.size()));
    
    RunnerInput input { "accounts", "createsession", {{ "username", username }}, // plainParams
        {{ "auth_password", password }}}; // dataParams

    if (!twofactor.empty()) 
        input.dataParams["auth_twofactor"] = twofactor; 
    MDBG_BACKEND(input);

    nlohmann::json resp(RunAction_Write(input));
    mDeleteSession = true;

    // TODO this is demo placeholder code for e2ee later... master_keywrap_salt comes from the server
    const std::string master_keywrap_salt { "\x7f\x1e\xc2\xb4\xf9\x09\xcc\xfb\xae\x64\x1d\xfd\x0f\x70\xb8\x05" };
    const SecureBuffer master_keywrap { Crypto::DeriveKey(password_cryptkey,master_keywrap_salt) };
    MDBG_INFO("... master_keywrap:"); mDebug.Info(mDebug.DumpBytes(master_keywrap.data(), master_keywrap.size()));

    try
    {
        resp.at("account").at("id").get_to(mAccountID);
        resp.at("client").at("session").at("id").get_to(mSessionID);
        resp.at("client").at("session").at("authkey").get_to(mSessionKey);

        MDBG_INFO("... accountID:" << mAccountID << " sessionID:" << mSessionID);
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
            PlatformUtil::SilentReadConsole(password);
        }

        try
        {
            Authenticate(username, password);
        }
        catch (const TwoFactorRequiredException&)
        {
            if (mOptions.quiet) throw; // rethrow

            std::string twofactor; std::cout << "Two Factor? ";
            PlatformUtil::SilentReadConsole(twofactor);

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
    nlohmann::json resp(RunAction_Read(input));

    try
    {
        resp.at("id").get_to(mAccountID);
        resp.at("username").get_to(mUsername);
    }
    catch (const nlohmann::json::exception& ex) {
        throw JSONErrorException(ex.what()); }
}

/*****************************************************/
void BackendImpl::PreAuthenticate(const SessionStore& session)
{
    // TODO in the future with the logged-out state, need to check for null ID/key... also will need to store the username!
    // or maybe the server could allow login via just the account ID? would be nicer

    PreAuthenticate(*session.GetSessionID(), *session.GetSessionKey());
}

/*****************************************************/
void BackendImpl::CloseSession()
{
    MDBG_INFO("()");
    
    if (mDeleteSession)
    {
        RunnerInput input {"accounts", "deleteclient"}; MDBG_BACKEND(input);
        RunAction_Write(input);
    }

    mDeleteSession = false;
    mUsername.clear();
    mSessionID.clear();
    mSessionKey.clear();
}

/*****************************************************/
void BackendImpl::StoreSession(SessionStore& sessionObj)
{
    if (mSessionID.empty()) sessionObj.SetSession(nullptr);
    else sessionObj.SetSession(mSessionID, mSessionKey);

    mDeleteSession = false;
}

/*****************************************************/
bool BackendImpl::isMemory() const
{
    return mOptions.cacheType == ConfigOptions::CacheType::MEMORY;
}

/*****************************************************/
nlohmann::json BackendImpl::GetCoreConfigJ()
{
    MDBG_INFO("()");


    RunnerInput input {"core", "getconfig"}; MDBG_BACKEND(input);
    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesConfigJ()
{
    MDBG_INFO("()");

    RunnerInput input {"files", "getconfig"}; MDBG_BACKEND(input);
    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetAccountLimits()
{
    if (mAccountID.empty())
        return nullptr;

    RunnerInput input {"files", "getlimits", {{"account", mAccountID}}}; MDBG_BACKEND(input);

    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFolder(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory()) // debug only
    {
        nlohmann::json retval;
        retval["id"] = id;
        retval["files"] = std::map<std::string,int>();
        retval["folders"] = std::map<std::string,int>();
        return retval;
    }

    RunnerInput input {"files", "getfolder", {{"folder", id}}}; MDBG_BACKEND(input);
    
    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFSRoot(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory()) // debug only
    {
        nlohmann::json retval;
        retval["id"] = id;
        retval["files"] = std::map<std::string,int>();
        retval["folders"] = std::map<std::string,int>();
        return retval;
    }

    RunnerInput input {"files", "getfolder", {{"filesystem", id}}}; MDBG_BACKEND(input);
    
    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesystem(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory() && id.empty()) return nullptr;

    RunnerInput input {"files", "getfilesystem", {{"filesystem", id}}}; MDBG_BACKEND(input);

    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFSLimits(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isMemory() && id.empty()) return nullptr;

    RunnerInput input {"files", "getlimits", {{"filesystem", id}}}; MDBG_BACKEND(input);

    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::GetFilesystems()
{
    MDBG_INFO("()");

    RunnerInput input {"files", "getfilesystems"}; MDBG_BACKEND(input);

    return RunAction_Read(input); 
}

/*****************************************************/
nlohmann::json BackendImpl::GetAdopted()
{
    MDBG_INFO("()");

    RunnerInput input {"files", "listadopted"}; MDBG_BACKEND(input);

    return RunAction_Read(input);
}

/*****************************************************/
nlohmann::json BackendImpl::CreateFile(const std::string& parent, const std::string& name, bool overwrite)
{
    MDBG_INFO("(parent:" << parent << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) // debug only
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"size", 0}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};
        
        return retval;
    }

    const std::string data; // empty - declare here as FilesIn takes a *reference*
    RunnerInput_FilesIn input {{"files", "upload", 
        {{"parent", parent}, {"overwrite", BOOLSTR(overwrite)}}}, // plainParams
        {{"file", {name, data}}}}; MDBG_BACKEND(input); // FilesIn
        
    return RunAction_FilesIn(input);
}

/*****************************************************/
nlohmann::json BackendImpl::CreateFolder(const std::string& parent, const std::string& name)
{
    MDBG_INFO("(parent:" << parent << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) // debug only
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};

        retval["files"] = std::map<std::string,int>(); 
        retval["folders"] = std::map<std::string,int>();

        return retval;
    }

    RunnerInput input {"files", "createfolder", {{"parent", parent}}, // plainParams
        {{"name", name}}}; MDBG_BACKEND(input); // dataParams

    return RunAction_Write(input);
}

/*****************************************************/
void BackendImpl::DeleteFile(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) return; // debug only

    RunnerInput input {"files", "deletefile", {{"file", id}}}; MDBG_BACKEND(input);
    
    try { RunAction_Write(input); }
    catch (const NotFoundException& e) {
        MDBG_INFO("... backend:" << e.what()); }
}

/*****************************************************/
void BackendImpl::DeleteFolder(const std::string& id)
{
    MDBG_INFO("(id:" << id << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) return; // debug only

    RunnerInput input {"files", "deletefolder", {{"folder", id}}}; MDBG_BACKEND(input);
    
    try { RunAction_Write(input); }
    catch (const NotFoundException& e) {
        MDBG_INFO("... backend:" << e.what()); }
}

/*****************************************************/
nlohmann::json BackendImpl::RenameFile(const std::string& id, const std::string& name, bool overwrite)
{
    MDBG_INFO("(id:" << id << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) return nullptr; // debug only

    RunnerInput input {"files", "renamefile", 
        {{"file", id}, {"overwrite", BOOLSTR(overwrite)}}, // plainParams
        {{"name", name}} }; MDBG_BACKEND(input); // dataParams

    return RunAction_Write(input);
}

/*****************************************************/
nlohmann::json BackendImpl::RenameFolder(const std::string& id, const std::string& name, bool overwrite)
{
    MDBG_INFO("(id:" << id << " name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) return nullptr; // debug only

    RunnerInput input {"files", "renamefolder", 
        {{"folder", id}, {"overwrite", BOOLSTR(overwrite)}}, // plainParams
        {{"name", name}} }; MDBG_BACKEND(input); // dataParams

    return RunAction_Write(input);
}

/*****************************************************/
nlohmann::json BackendImpl::MoveFile(const std::string& id, const std::string& parent, bool overwrite)
{
    MDBG_INFO("(id:" << id << " parent:" << parent << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) return nullptr; // debug only

    RunnerInput input {"files", "movefile", 
        {{"file", id}, {"parent", parent}, {"overwrite", BOOLSTR(overwrite)}}}; MDBG_BACKEND(input); // plainParams

    return RunAction_Write(input);
}

/*****************************************************/
nlohmann::json BackendImpl::MoveFolder(const std::string& id, const std::string& parent, bool overwrite)
{
    MDBG_INFO("(id:" << id << " parent:" << parent << ")");

    if (isReadOnly()) throw ReadOnlyException();

    if (isMemory()) return nullptr; // debug only

    RunnerInput input {"files", "movefolder", 
        {{"folder", id}, {"parent", parent}, {"overwrite", BOOLSTR(overwrite)}}}; MDBG_BACKEND(input); // plainParams

    return RunAction_Write(input);
}

/*****************************************************/
std::string BackendImpl::ReadFile(const std::string& id, const uint64_t offset, const size_t length)
{
    if (!length) { MDBG_ERROR("() ERROR 0 length"); assert(false); return ""; }

    const std::string fstart(std::to_string(offset));
    const std::string flast(std::to_string(offset+length-1));

    MDBG_INFO("(id:" << id << " fstart:" << fstart << " flast:" << flast << ")");

    if (isMemory()) return std::string(length,'\0'); // debug only

    RunnerInput input {"files", "download", {{"file", id}}, // plainParams
        {{"fstart", fstart}, {"flast", flast}}}; MDBG_BACKEND(input); // dataParams

    std::string data { RunAction_ReadStr(input) }; // non-const for move
    if (data.size() != length) throw ReadSizeException(length, data.size());
    return data;
}

/*****************************************************/
void BackendImpl::ReadFile(const std::string& id, const uint64_t offset, const size_t length, const ReadFunc& userFunc)
{
    if (!length) { MDBG_ERROR("() ERROR 0 length"); assert(false); return; }

    const std::string fstart(std::to_string(offset));
    const std::string flast(std::to_string(offset+length-1));

    MDBG_INFO("(id:" << id << " fstart:" << fstart << " flast:" << flast << ")");

    if (isMemory()) { userFunc(0, std::string(length,'\0').data(), length); return; } // debug only

    size_t read = 0; RunnerInput_StreamOut input {{"files", "download", {{"file", id}}, // plainParams
        {{"fstart", fstart}, {"flast", flast}}}, // dataParams
        [&](const size_t soffset, const char* buf, const size_t buflen)->void
    {
        if (soffset+buflen > length) // too much data
            throw ReadSizeException(length, soffset+buflen);
        
        read = std::max(read, soffset+buflen);
        userFunc(soffset, buf, buflen); 
    }}; MDBG_BACKEND(input);

    RunAction_StreamOut(input);
    if (read < length) throw ReadSizeException(length, read);
}

namespace { // anonymous
// if we get a 413 this small the server must be bugged
constexpr size_t UPLOAD_MINSIZE { 4096 };
// multiply the failed by this to get the next attempt size
constexpr size_t ADJUST_ATTEMPT(size_t maxSize){ return maxSize/2; }
} // namespace

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

    if (isMemory()) return nullptr; // debug only

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

    if (isMemory()) // debug only
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, 
            {"size", RunnerInput_StreamIn::StreamSize(userFunc)}, {"filesystem", ""}};
        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};
        return retval;
    }

    return SendFile(userFunc, "", 0, [&](const WriteFunc& writeFunc)->RunnerInput_StreamIn
    {
        return {{{"files", "upload", 
            {{"parent", parent}, {"overwrite", BOOLSTR(overwrite)}}}}, // plainParams
            {{"file", {name, writeFunc}}}}; // StreamIn
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
                else { sread = 0; return false; } // end of chunk
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
            input = {{{"files", "writefile", {{"file", id}}, // plainParams
                {{"offset", std::to_string(offset+byte)}}}}, {{"data", {"data", writeFunc}}}}; // dataParams, StreamIn
        }
        MDBG_BACKEND(input);

        try
        {
            retval = RunAction_StreamIn(input);
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

    if (isMemory()) return nullptr; // debug only

    RunnerInput input {"files", "ftruncate", {{"file", id}}, // plainParams
        {{"size", std::to_string(size)}}}; MDBG_BACKEND(input); // dataParams

    return RunAction_Write(input);
}

} // namespace Backend
} // namespace Andromeda
