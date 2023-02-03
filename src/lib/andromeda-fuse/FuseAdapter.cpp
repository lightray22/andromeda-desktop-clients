
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>

#ifdef APPLE
#include <sys/mount.h> // unmount
#endif

#include "libfuse_Includes.h"
#include "FuseAdapter.hpp"
#include "FuseOperations.hpp"

#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;

static a2fuse_operations a2fuse_ops;
static Debug sDebug("FuseAdapter",nullptr);

typedef std::unique_lock<std::mutex> UniqueLock;

namespace AndromedaFuse {

/*****************************************************/
/** Scope-managed fuse_args */
struct FuseArguments
{
    FuseArguments(): mFuseArgs(FUSE_ARGS_INIT(0,NULL))
    {
        SDBG_INFO("() fuse_opt_add_arg()");

        int retval; if ((retval = fuse_opt_add_arg(&mFuseArgs, "andromeda-fuse")) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()1 failed: "+std::to_string(retval));
    };

    ~FuseArguments()
    { 
        SDBG_INFO("() fuse_opt_free_args()");

        fuse_opt_free_args(&mFuseArgs); 
    };

    /** @param arg -o fuse argument */
    void AddArg(const std::string& arg)
    { 
        SDBG_INFO("(arg:" << arg << ")"); int retval;

        if ((retval = fuse_opt_add_arg(&mFuseArgs, "-o")) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()2 failed: "+std::to_string(retval));

        if ((retval = fuse_opt_add_arg(&mFuseArgs, arg.c_str())) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()3 failed: "+std::to_string(retval));
    };
    /** fuse_args struct */
    struct fuse_args mFuseArgs;
};

/*****************************************************/
/** Run fuse_daemonize (detach from terminal) */
static void FuseDaemonize()
{
    SDBG_INFO("... fuse_daemonize()");

    int retval; if ((retval = fuse_daemonize(0)) != FUSE_SUCCESS)
        throw FuseAdapter::Exception("fuse_daemonize() failed: "+std::to_string(retval));
}

#if LIBFUSE2

/*****************************************************/
/** fuse_mount (manual unmount, not scope-managed) */
struct FuseMount
{
    /** @param fargs FuseArguments reference
      * @param path filesystem path to mount */
    FuseMount(FuseArguments& fargs, const char* const path): mPath(path)
    {
        SDBG_INFO("() fuse_mount(path:" << path << ")");

        mFuseChan = fuse_mount(mPath, &fargs.mFuseArgs);
        if (!mFuseChan) throw FuseAdapter::Exception("fuse_mount() failed");
    };

    /** Unmount the FUSE session */
    void Unmount()
    {
        if (mFuseChan == nullptr) return;
        mFuseChan = nullptr;

        SDBG_INFO("() fuse_unmount()");
        fuse_unmount(mPath, mFuseChan); 
    };

    /** mounted path */
    const char* const mPath;
    /** fuse_chan pointer */
    struct fuse_chan* mFuseChan;
};

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy (also unmounts) */
struct FuseContext
{
    /** @param mount FuseMount reference 
      * @param fargs FuseArguments reference */
    FuseContext(FuseAdapter& adapter, FuseMount& mount, FuseArguments& fargs): 
        mAdapter(adapter), mMount(mount)
    { 
        SDBG_INFO("() fuse_new()");

        mFuse = fuse_new(mMount.mFuseChan, &(fargs.mFuseArgs), 
            &a2fuse_ops, sizeof(a2fuse_ops), static_cast<void*>(&adapter));
        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");

        mAdapter.mFuseContext = this; // register with adapter
    };

    ~FuseContext()
    {
        mAdapter.mFuseContext = nullptr;

        SDBG_INFO("()");
        mMount.Unmount(); 

        SDBG_INFO("... fuse_destroy()");
        fuse_destroy(mFuse);
    };

    /** Exit and unmount the FUSE session */
    void TriggerUnmount()
    {
        SDBG_INFO("() fuse_exit()");
        fuse_exit(mFuse); // flag loop to stop

        // fuse_exit does not interrupt the loop, so to prevent it hanging until the next FS operation
        // we will send off a umount command... ugly, but see https://github.com/winfsp/cgofuse/issues/6#issuecomment-298185815
        // fuse_unmount() is not valid on this thread, and can't use unmount() library call as that requires superuser

        #if APPLE
            // OSX hangs our whole process (not just this thread) 60 seconds if we start a
            // background process but fortunately it allows unmount() without being a superuser
            SDBG_INFO("... calling unmount(2)");
            unmount(mMount.mPath, MNT_FORCE);
            SDBG_INFO("... unmount returned");
        #else
            std::stringstream cmd;
            cmd << "umount \"" << mMount.mPath << "\"&";

            SDBG_INFO("... " << cmd.str());
            int retval { std::system(cmd.str().c_str()) }; // can fail
            SDBG_INFO("... system returned: " << retval);
        #endif
    }

    FuseAdapter& mAdapter;
    /** Fuse context pointer */
    struct fuse* mFuse;
    /** Fuse mount reference */
    FuseMount& mMount;
};

#else // !LIBFUSE2

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param fargs FuseArguments reference */
    FuseContext(FuseAdapter& adapter, FuseArguments& fargs)
    {
        SDBG_INFO("() fuse_new()");

        mFuse = fuse_new(&(fargs.mFuseArgs), 
            &a2fuse_ops, sizeof(a2fuse_ops), static_cast<void*>(&adapter));
        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");
    };

    ~FuseContext()
    {
        SDBG_INFO("() fuse_destroy()");
        fuse_destroy(mFuse);
    };

    /** Fuse context pointer */
    struct fuse* mFuse;
};

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    /** @param context FuseContext reference
      * @param path filesystem path to mount */
    FuseMount(FuseAdapter& adapter, FuseContext& context, const char* const path): 
        mAdapter(adapter), mContext(context), mPath(path)
    {
        SDBG_INFO("() fuse_mount(path:" << path << ")");

        int retval; if ((retval = fuse_mount(mContext.mFuse, path)) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_mount() failed: "+std::to_string(retval));
        mAdapter.mFuseMount = this; // register with adapter
    };

    /** Exit and unmount the FUSE session */
    void TriggerUnmount()
    {
        SDBG_INFO("() fuse_exit()");
        fuse_exit(mContext.mFuse);// flag loop to stop

        // fuse_exit does not interrupt the loop (except WinFSP), so to prevent it hanging until the next FS operation
        // we will send off a umount command... ugly, but see https://github.com/winfsp/cgofuse/issues/6#issuecomment-298185815
        // fuse_unmount() is not valid on this thread, and can't use unmount() library call as that requires superuser
        
        #if !WIN32
            std::stringstream cmd;
            cmd << "umount \"" << mPath << "\"&";

            SDBG_INFO("... " << cmd.str());
            int retval { std::system(cmd.str().c_str()) }; // can fail
            SDBG_INFO("... system returned: " << retval);
        #endif
    }

    ~FuseMount() 
    {
        mAdapter.mFuseMount = nullptr;

        SDBG_INFO("() fuse_unmount()");
        fuse_unmount(mContext.mFuse);
    }

    FuseAdapter& mAdapter;
    FuseContext& mContext;
    /** mounted path */
    const char* const mPath;
};

#endif // LIBFUSE2

/*****************************************************/
/** Scope-managed fuse_set/remove_signal_handlers */
struct FuseSignals
{
    /** @param context FuseContext reference */
    explicit FuseSignals(FuseContext& context)
    { 
        SDBG_INFO("() fuse_set_signal_handlers()");

        mFuseSession = fuse_get_session(context.mFuse);

        int retval; if ((retval = fuse_set_signal_handlers(mFuseSession)) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_set_signal_handlers() failed: "+std::to_string(retval));
    };

    ~FuseSignals()
    {
        SDBG_INFO("() fuse_remove_signal_handlers()");

        fuse_remove_signal_handlers(mFuseSession); 
    }

    /** fuse_session pointer */
    struct fuse_session* mFuseSession;
};

/*****************************************************/
FuseAdapter::FuseAdapter(const std::string& mountPath, Folder& root, const FuseOptions& options, RunMode runMode)
    : mMountPath(mountPath), mRootFolder(root), mOptions(options)
{
    SDBG_INFO("(path:" << mMountPath << ")");

    if (runMode == RunMode::THREAD)
    {
        mFuseThread = std::thread(&FuseAdapter::RunFuse, this, runMode);

        SDBG_INFO("... waiting for init");

        UniqueLock initLock(mInitMutex);
        while (!mInitialized) mInitCV.wait(initLock);

        SDBG_INFO("... init complete!");
    }
    else RunFuse(runMode); // blocking

    if (mInitError)
    {
        if (runMode == RunMode::THREAD)
            mFuseThread.join();
        
        std::rethrow_exception(mInitError);
    }
}

/*****************************************************/
void FuseAdapter::RunFuse(RunMode runMode)
{
    SDBG_INFO("()");

    try
    {
        FuseArguments fuseArgs; 
    #if WIN32
        // For WinFSP, use the current user
        fuseArgs.AddArg("uid=-1,gid=-1");
    #endif // WIN32
        for (const std::string& fuseArg : mOptions.fuseArgs)
            fuseArgs.AddArg(fuseArg);

    #if LIBFUSE2
        FuseMount mount(fuseArgs, mMountPath.c_str());
        FuseContext context(*this, mount, fuseArgs);
    #else // !LIBFUSE2
        FuseContext context(*this, fuseArgs);
        FuseMount mount(*this, context, mMountPath.c_str());
    #endif // LIBFUSE2
        
        if (runMode == RunMode::DAEMON) FuseDaemonize();

        std::unique_ptr<FuseSignals> signals; 
        if (runMode != RunMode::THREAD) 
            signals = std::make_unique<FuseSignals>(context);
        
        SDBG_INFO("() fuse_loop()");

        int retval; if ((retval = fuse_loop(context.mFuse)) < 0)
            throw FuseAdapter::Exception("fuse_loop() failed: "+std::to_string(retval));

        SDBG_INFO("() fuse_loop() returned!");
    }
    catch (const Exception& ex)
    {
        SDBG_ERROR("... error: " << ex.what());
        mInitError = std::current_exception();
    }

    SignalInit(); // just in case fuse fails but doesn't throw
}

/*****************************************************/
void FuseAdapter::SignalInit()
{
    SDBG_INFO("()");

    UniqueLock initLock(mInitMutex);
    mInitialized = true; mInitCV.notify_all();
}

/*****************************************************/
FuseAdapter::~FuseAdapter()
{
    SDBG_INFO("()");
    
#if LIBFUSE2
    if (mFuseContext) 
        mFuseContext->TriggerUnmount();
#else // !LIBFUSE2
    if (mFuseMount) 
        mFuseMount->TriggerUnmount();
#endif // LIBFUSE2

    if (mFuseThread.joinable())
    {
        SDBG_INFO("... waiting");
        mFuseThread.join();
    }

    SDBG_INFO("... return!");
}

/*****************************************************/
void FuseAdapter::ShowVersionText()
{
    std::cout << "libfuse version: " << fuse_version()
#if !LIBFUSE2
        << " (" << fuse_pkgversion() << ")"
#endif // !LIBFUSE2
        << std::endl;

#if !LIBFUSE2 && !WIN32
    fuse_lowlevel_version();
#endif // !LIBFUSE2
}

} // namespace AndromedaFuse
