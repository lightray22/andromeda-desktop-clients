#ifndef A2GUI_BACKENDCONTEXT_H
#define A2GUI_BACKENDCONTEXT_H

#include <memory>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"
#include "andromeda/backend/RunnerOptions.hpp"

namespace Andromeda { 
    namespace Backend { 
        class BackendImpl; class HTTPRunner; class RunnerPool; class SessionStore; }
    namespace Database { class ObjectDatabase; } }

namespace AndromedaGui {

/** Encapsulates a backend and its resources */
class BackendContext
{
public:

    /** Create a new BackendContext from user input (sessionstore is nullptr) */
    BackendContext(const std::string& url, const std::string& username, 
        const std::string& password, const std::string& twofactor);

    /** Create a new BackendContext from a known session and store ref */
    BackendContext(Andromeda::Backend::SessionStore& session);

    virtual ~BackendContext();
    DELETE_COPY(BackendContext)
    DELETE_MOVE(BackendContext)

    /** Returns the Backend instance */
    inline Andromeda::Backend::BackendImpl& GetBackend() { return *mBackend; }
    /** Returns the HTTPRunner instance */
    inline Andromeda::Backend::HTTPRunner& GetRunner() { return *mRunner; }

    /** 
     * Creates a new session store and stores it here
     * @throws DatabaseException
     */
    void StoreSession(Andromeda::Database::ObjectDatabase& objdb);
    /** Returns the SessionStore instance or nullptr if not set */
    inline Andromeda::Backend::SessionStore* GetSessionStore() const { return mSessionStore; }

private:

    /** Create the backend and runner objects */
    void InitializeBackend(const std::string& url);

    /** Returns the andromeda-gui http user agent */
    static std::string GetUserAgent();

    mutable Andromeda::Debug mDebug;

    /** libandromeda configuration */
    Andromeda::ConfigOptions mConfigOptions;
    /** HTTP Runner configuration */
    Andromeda::Backend::HTTPOptions mHttpOptions;
    Andromeda::Backend::RunnerOptions mRunnerOptions;
    
    std::unique_ptr<Andromeda::Backend::HTTPRunner> mRunner;
    std::unique_ptr<Andromeda::Backend::RunnerPool> mRunners;
    std::unique_ptr<Andromeda::Backend::BackendImpl> mBackend;

    Andromeda::Backend::SessionStore* mSessionStore { nullptr };
};

} // namespace AndromedaGui

#endif // A2GUI_BACKENDCONTEXT_H
