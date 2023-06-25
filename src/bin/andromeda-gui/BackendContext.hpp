#ifndef A2GUI_BACKENDCONTEXT_H
#define A2GUI_BACKENDCONTEXT_H

#include <memory>

#include "andromeda/Debug.hpp"
#include "andromeda/Utilities.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"
#include "andromeda/backend/RunnerOptions.hpp"

namespace Andromeda { namespace Backend { 
    class BackendImpl; class HTTPRunner; class RunnerPool; } }

namespace AndromedaGui {

/** Encapsulates a backend and its resources */
class BackendContext
{
public:

    /** Create a new BackendContext from user input */
    BackendContext(
        const std::string& url, const std::string& username, 
        const std::string& password, const std::string& twofactor);

    virtual ~BackendContext();
    DELETE_COPY(BackendContext)
    DELETE_MOVE(BackendContext)

    /** Returns the Andromeda::Backend instance */
    Andromeda::Backend::BackendImpl& GetBackend() { return *mBackend; }

private:

    /** libandromeda configuration */
    Andromeda::ConfigOptions mConfigOptions;
    /** HTTP Runner configuration */
    Andromeda::Backend::HTTPOptions mHttpOptions;
    Andromeda::Backend::RunnerOptions mRunnerOptions;
    
    std::unique_ptr<Andromeda::Backend::HTTPRunner> mRunner;
    std::unique_ptr<Andromeda::Backend::RunnerPool> mRunners;
    std::unique_ptr<Andromeda::Backend::BackendImpl> mBackend;

    mutable Andromeda::Debug mDebug;
};

} // namespace AndromedaGui

#endif // A2GUI_BACKENDCONTEXT_H
