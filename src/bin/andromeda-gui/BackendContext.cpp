
#include "BackendContext.hpp"

#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/RunnerPool.hpp"
using Andromeda::Backend::RunnerPool;

namespace AndromedaGui {

/*****************************************************/
BackendContext::BackendContext(
    const std::string& url, const std::string& username, 
    const std::string& password, const std::string& twofactor) : 
    mDebug(__func__,this) 
{
    MDBG_INFO("(url:" << url << ", username:" << username << ")");

    HTTPRunner::HostUrlPair urlPair { HTTPRunner::ParseURL(url) };

    mRunner = std::make_unique<HTTPRunner>(
        urlPair.first, urlPair.second, mHttpOptions);

    mRunners = std::make_unique<RunnerPool>(*mRunner, mConfigOptions);

    mBackend = std::make_unique<BackendImpl>(mConfigOptions, *mRunners);
    BackendImpl& backend { *mBackend }; // context will get moved

    backend.Authenticate(username, password, twofactor);

    mRunner->EnableRetry(); // no retry during init
}

/*****************************************************/
BackendContext::~BackendContext()
{
    MDBG_INFO("()");
}

} // namespace AndromedaGui
