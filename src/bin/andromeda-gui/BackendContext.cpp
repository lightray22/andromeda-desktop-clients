
#include "BackendContext.hpp"

#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;

/*****************************************************/
BackendContext::BackendContext(
    const std::string& url, const std::string& username, 
    const std::string& password, const std::string& twofactor) : 
    mDebug("BackendContext",this) 
{
    mDebug << __func__ << "(url:" << url << ", username:" << username << ")"; mDebug.Info();

    HTTPRunner::HostUrlPair urlPair { HTTPRunner::ParseURL(url) };

    mRunner = std::make_unique<HTTPRunner>(
        urlPair.first, urlPair.second, mHttpOptions);

    mBackend = std::make_unique<BackendImpl>(mConfigOptions, *mRunner);
    BackendImpl& backend { *mBackend }; // context will get moved

    backend.Initialize();
    backend.Authenticate(username, password, twofactor);

    mRunner->EnableRetry(); // no retry during init
}

/*****************************************************/
BackendContext::~BackendContext()
{
    mDebug << __func__ << "()"; mDebug.Info();
}
