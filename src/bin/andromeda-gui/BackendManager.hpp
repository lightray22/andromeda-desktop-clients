#ifndef A2GUI_BACKENDMANAGER_H
#define A2GUI_BACKENDMANAGER_H

#include <memory>
#include <vector>

#include "andromeda/Backend.hpp"
#include "andromeda/Config.hpp"
#include "andromeda/HTTPRunner.hpp"
#include "andromeda/Utilities.hpp"

// TODO comments (here + all functions)
class BackendManager
{
public:

    BackendManager();

    virtual ~BackendManager();

    bool HasBackend();

    Andromeda::Backend& AddBackend(
        const std::string& url, const std::string& username, 
        const std::string& password, const std::string& twofactor);

private:

    struct BackendContext
    {
        Andromeda::Config::Options configOptions;
        Andromeda::HTTPRunner::Options httpOptions;
        std::unique_ptr<Andromeda::HTTPRunner> runner;
        std::unique_ptr<Andromeda::Backend> backend;
    };

    std::vector<BackendContext> mContexts;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_MOUNTMANAGER_H
