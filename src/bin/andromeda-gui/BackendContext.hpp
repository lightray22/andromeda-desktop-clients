#ifndef A2GUI_BACKENDCONTEXT_H
#define A2GUI_BACKENDCONTEXT_H

#include <memory>

#include "andromeda/Debug.hpp"
#include "andromeda/backend/ConfigOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"

namespace Andromeda { namespace Backend { class BackendImpl; class HTTPRunner; } }

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

    /** Returns the Andromeda::Backend instance */
    Andromeda::Backend::BackendImpl& GetBackend() { return *mBackend; }

private:

    /** libandromeda configuration */
    Andromeda::Backend::ConfigOptions mConfigOptions;
    /** HTTP Runner configuration */
    Andromeda::Backend::HTTPOptions mHttpOptions;
    
    std::unique_ptr<Andromeda::Backend::HTTPRunner> mRunner;
    std::unique_ptr<Andromeda::Backend::BackendImpl> mBackend;

    Andromeda::Debug mDebug;
};

} // namespace AndromedaGui

#endif // A2GUI_BACKENDCONTEXT_H
