
#include "BackendContext.hpp"

#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/RunnerPool.hpp"
using Andromeda::Backend::RunnerPool;
#include "andromeda/backend/SessionStore.hpp"
using Andromeda::Backend::SessionStore;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;

namespace AndromedaGui {

/*****************************************************/
BackendContext::BackendContext(
    const std::string& url, const std::string& username, 
    const std::string& password, const std::string& twofactor) : 
    mDebug(__func__,this) 
{
    MDBG_INFO("(url:" << url << ", username:" << username << ")");

    InitializeBackend(url);

    mBackend->Authenticate(username, password, twofactor);
    mRunner->EnableRetry(); // no retry during init
}

/*****************************************************/
BackendContext::BackendContext(SessionStore& session) : 
    mDebug(__func__,this), mSessionStore(&session)
{
    MDBG_INFO("(url:" << session.GetServerUrl() << ")");

    InitializeBackend(session.GetServerUrl());

    mBackend->PreAuthenticate(session);
    mRunner->EnableRetry(); // no retry during init
}

/*****************************************************/
BackendContext::~BackendContext()
{
    MDBG_INFO("()");
}

/*****************************************************/
void BackendContext::StoreSession(ObjectDatabase& objdb)
{
    mSessionStore = &SessionStore::Create(objdb, 
        mRunner->GetFullURL(), mBackend->GetAccountID());

    mBackend->StoreSession(*mSessionStore);
    mSessionStore->Save(); // store to DB
}

/*****************************************************/
void BackendContext::InitializeBackend(const std::string& url)
{
    mRunner = std::make_unique<HTTPRunner>(url, 
        GetUserAgent(), mRunnerOptions, mHttpOptions);

    mRunners = std::make_unique<RunnerPool>(*mRunner, mConfigOptions);

    mBackend = std::make_unique<BackendImpl>(mConfigOptions, *mRunners);
}

/*****************************************************/
std::string BackendContext::GetUserAgent()
{
    return std::string("andromeda-gui/")+ANDROMEDA_VERSION+"/"+SYSTEM_NAME;
}

} // namespace AndromedaGui
