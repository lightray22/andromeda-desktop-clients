
#include "BackendManager.hpp"

#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda/HTTPRunner.hpp"
using Andromeda::HTTPRunner;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;

/*****************************************************/
BackendManager::BackendManager() : 
    mDebug("BackendManager") 
{ 
    mDebug << __func__ << "()"; mDebug.Info();
}

/*****************************************************/
BackendManager::~BackendManager()
{
    mDebug << __func__ << "()"; mDebug.Info();
}

/*****************************************************/
bool BackendManager::HasBackend()
{
    return !mContexts.empty();
}

/*****************************************************/
Backend& BackendManager::AddBackend(
    const std::string& url, const std::string& username, 
    const std::string& password, const std::string& twofactor)
{
    mDebug << __func__ << "(url:" << url << ", username:" << username << ")"; mDebug.Info();

    BackendContext context;

    HTTPRunner::HostUrlPair urlPair { HTTPRunner::ParseURL(url) };

    context.runner = std::make_unique<HTTPRunner>(
        urlPair.first, urlPair.second, context.httpOptions);

    context.backend = std::make_unique<Backend>(*context.runner);
    Backend& backend { *context.backend }; // context will get moved

    context.backend->Initialize(context.configOptions);
    context.backend->Authenticate(username, password, twofactor);

    context.runner->EnableRetry(); // no retry during init

    mContexts.emplace_back(std::move(context)); return backend;
}
