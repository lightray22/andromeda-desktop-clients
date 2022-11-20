#ifndef A2FUSE_FUSEADAPTER_H_
#define A2FUSE_FUSEADAPTER_H_

#include <condition_variable>
#include <exception>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "andromeda/Utilities.hpp"

namespace Andromeda {
namespace FSItems { 
    class Folder;
} // nanespace FSItems
} // namespace Andromeda

namespace AndromedaFuse {

struct FuseLoop;
struct FuseOperations;

/** Static class for FUSE operations */
class FuseAdapter
{
public:

    /** Base Exception for all FUSE issues */
    class Exception : public Andromeda::Utilities::Exception { public:
        /** @param message FUSE error message */
        explicit Exception(const std::string& message) :
            Andromeda::Utilities::Exception("FUSE Error: "+message) {}; };

    /** FUSE wrapper options */
    struct Options
    {
        /** The FUSE directory to mount */
        std::string mountPath;

        /** List of FUSE library options */
        std::list<std::string> fuseArgs;

        /** Whether fake chmod (no-op) is allowed */
        bool fakeChmod { true };

        /** Whether fake chown (no-op) is allowed */
        bool fakeChown { true };
    };

    /** Thread mode for the FUSE Adapter */
    enum class RunMode
    {
        /** Run in the foreground (block) */
        FOREGROUND,
        /** Run in the foreground but detach from the terminal */
        DAEMON,
        /** Run in a background thread (don't block) */
        THREAD
    };

    /**
     * Starts and mounts libfuse
     * @param root andromeda folder as root
     * @param options command line options (copied)
     * @param runMode threading mode
     */
    FuseAdapter(Andromeda::FSItems::Folder& root, const Options& options, RunMode runMode);

    /** Stop and unmount FUSE */
    virtual ~FuseAdapter();

    /** Returns the root folder */
    Andromeda::FSItems::Folder& GetRootFolder(){ return mRootFolder; }

    /** Returns the FUSE options */
    const Options& GetOptions(){ return mOptions; }

    /** Print help text to stdout */
    static void ShowHelpText();

    /** Print version text to stdout */
    static void ShowVersionText();

private:

    friend struct FuseLoop;
    friend struct FuseOperations;

    /** Runs/mounts libfuse (blocking) */
    void RunFuse(RunMode runMode);

    /** Signals initialization complete */
    void SignalInit();
    
    Andromeda::FSItems::Folder& mRootFolder;
    Options mOptions;

    std::thread mFuseThread;
    FuseLoop* mFuseLoop { nullptr }; // TODO should probably be locked?

    bool mInitialized { false };
    std::mutex mInitMutex;
    std::condition_variable mInitCV;
    std::exception_ptr mInitError;

    FuseAdapter(const FuseAdapter&) = delete; // no copying
};

} // namespace AndromedaFuse

#endif // A2FUSE_FUSEADAPTER_H
