
#include <iostream>
#include <memory>

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
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    /** @param fargs FuseArguments reference
     * @param path filesystem path to mount */
    FuseMount(FuseArguments& fargs, const char* const path): mPath(path)
    {
        SDBG_INFO("() fuse_mount()");
        
        mFuseChan = fuse_mount(mPath, &fargs.mFuseArgs);
        
        if (!mFuseChan) throw FuseAdapter::Exception("fuse_mount() failed");
    };

    void Unmount()
    {
        if (mFuseChan == nullptr) return;

        SDBG_INFO("() fuse_unmount()");
        
        fuse_unmount(mPath, mFuseChan); mFuseChan = nullptr;
    };

    ~FuseMount(){ Unmount(); };
    
    /** mounted path */
    const char* const mPath;
    /** fuse_chan pointer */
    struct fuse_chan* mFuseChan;
};

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param mount FuseMount reference 
     * @param fargs FuseArguments reference */
    FuseContext(FuseMount& mount, FuseArguments& fargs, FuseAdapter& adapter): mMount(mount)
    { 
        SDBG_INFO("() fuse_new()");

        mFuse = fuse_new(mMount.mFuseChan, &(fargs.mFuseArgs), 
            &a2fuse_ops, sizeof(a2fuse_ops), static_cast<void*>(&adapter));
        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");
    };

    ~FuseContext()
    {
        mMount.Unmount(); 

        SDBG_INFO("() fuse_destroy()");

        fuse_destroy(mFuse);
    };

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
    FuseContext(FuseArguments& fargs, FuseAdapter& adapter)
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
    FuseMount(FuseContext& context, const char* path): mContext(context)
    {
        SDBG_INFO("() fuse_mount()");

        int retval; if ((retval = fuse_mount(mContext.mFuse, path)) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_mount() failed: "+std::to_string(retval));
    };

    ~FuseMount()
    {
        SDBG_INFO("() fuse_unmount()");
        
        fuse_unmount(mContext.mFuse);
    }

    FuseContext& mContext;
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
/** Scope-managed fuse event loop - blocks */
struct FuseLoop
{
    /** @param context FUSE context reference */
    explicit FuseLoop(FuseContext& context, FuseAdapter& adapter): 
        mContext(context), mAdapter(adapter)
    {
        SDBG_INFO("() fuse_loop()");

        { // scoped lock
            const UniqueLock lock(mAdapter.mFuseLoopMutex);
            mAdapter.mFuseLoop = this; // register with adapter
        }
        
        int retval; if ((retval = fuse_loop(context.mFuse)) < 0)
            throw FuseAdapter::Exception("fuse_loop() failed: "+std::to_string(retval));

        SDBG_INFO("() fuse_loop() returned!");
    }

    ~FuseLoop()
    {
        const UniqueLock lock(mAdapter.mFuseLoopMutex);
        mAdapter.mFuseLoop = nullptr;
    }

    /** Flags the fuse session to terminate */
    void ExitLoop() 
    {
        SDBG_INFO("()");

        fuse_exit(mContext.mFuse); 
    }

    FuseContext& mContext;
    FuseAdapter& mAdapter;
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
        for (const std::string& fuseArg : mOptions.fuseArgs)
            fuseArgs.AddArg(fuseArg);

    #if LIBFUSE2
        FuseMount mount(fuseArgs, mMountPath.c_str());
        FuseContext context(mount, fuseArgs, *this);
    #else // !LIBFUSE2
        FuseContext context(fuseArgs, *this);
        FuseMount mount(context, mMountPath.c_str());
    #endif // LIBFUSE2
        
        if (runMode == RunMode::DAEMON) FuseDaemonize();

        std::unique_ptr<FuseSignals> signals; 
        if (runMode != RunMode::THREAD) 
            signals = std::make_unique<FuseSignals>(context);
        
        FuseLoop loop(context, *this); // blocks until done
    }
    catch (const Exception& ex)
    {
        SDBG_ERROR("... error: " << ex.what());
        mInitError = std::current_exception();
    }

    SignalInit(); // outside catch just in case fuse fails but doesn't throw
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
    
    { // scoped lock
        const UniqueLock lock(mFuseLoopMutex);
        if (mFuseLoop) mFuseLoop->ExitLoop();
    }

    SDBG_INFO("... waiting");

    if (mFuseThread.joinable())
        mFuseThread.join();

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
