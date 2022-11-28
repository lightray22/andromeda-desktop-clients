#ifndef A2GUI_BACKENDCONTEXT_H
#define A2GUI_BACKENDCONTEXT_H

#include <memory>

#include "andromeda/Config.hpp"
#include "andromeda/HTTPRunner.hpp"
#include "andromeda/Utilities.hpp"

namespace Andromeda { class Backend; }

// TODO comments (here + all functions)
class BackendContext
{
public:

    BackendContext(
        const std::string& url, const std::string& username, 
        const std::string& password, const std::string& twofactor);

    virtual ~BackendContext();

    Andromeda::Backend& GetBackend() { return *mBackend; }

private:

    Andromeda::Config::Options mConfigOptions;
    Andromeda::HTTPRunner::Options mHttpOptions;

    std::unique_ptr<Andromeda::HTTPRunner> mRunner;
    std::unique_ptr<Andromeda::Backend> mBackend;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_BACKENDCONTEXT_H
