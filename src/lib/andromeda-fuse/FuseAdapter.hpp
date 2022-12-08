#ifndef A2FUSE_FUSEADAPTER_H_
#define A2FUSE_FUSEADAPTER_H_

#include <condition_variable>
#include <exception>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "FuseOptions.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {
namespace Filesystem { 
    class Folder;
} // nanespace Filesystem
} // namespace Andromeda

namespace AndromedaFuse {

struct FuseLoop;
struct FuseOperations;

/** Static class for FUSE operations */
class FuseAdapter
{
public:

    /** Base Exception for all FUSE issues */
    class Exception : public Andromeda::BaseException { public:
        /** @param message FUSE error message */
        explicit Exception(const std::string& message) :
            Andromeda::BaseException("FUSE Error: "+message) {}; };

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
     * @param path filesystem path to mount
     * @param root andromeda folder as root
     * @param options command line options (copied)
     * @param runMode threading mode
     */
    FuseAdapter(
        const std::string& path, Andromeda::Filesystem::Folder& root, 
        const FuseOptions& options, RunMode runMode);

    /** Stop and unmount FUSE */
    virtual ~FuseAdapter();

    /** Returns the mounted filesystem path */
    const std::string& GetMountPath() const { return mMountPath; }

    /** Returns the root folder */
    Andromeda::Filesystem::Folder& GetRootFolder() { return mRootFolder; }

    /** Returns the FUSE options */
    const FuseOptions& GetOptions() const { return mOptions; }

    /** Print version text to stdout */
    static void ShowVersionText();

private:

    friend struct FuseLoop;
    friend struct FuseOperations;

    /** Runs/mounts libfuse (blocking) */
    void RunFuse(RunMode runMode);

    /** Signals initialization complete */
    void SignalInit();
    
    std::string mMountPath;
    Andromeda::Filesystem::Folder& mRootFolder;
    FuseOptions mOptions;

    std::thread mFuseThread;
    FuseLoop* mFuseLoop { nullptr };
    std::mutex mFuseLoopMutex;

    bool mInitialized { false };
    std::mutex mInitMutex;
    std::condition_variable mInitCV;
    std::exception_ptr mInitError;

    FuseAdapter(const FuseAdapter&) = delete; // no copying
};

} // namespace AndromedaFuse

#endif // A2FUSE_FUSEADAPTER_H
