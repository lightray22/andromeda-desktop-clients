
#include <iostream>
#include <memory>

#include "libfuse_Includes.h"
#include "FuseAdapter.hpp"
#include "FuseOperations.hpp"

#include "andromeda/Utilities.hpp"
using Andromeda::Debug;
#include "andromeda/fsitems/Folder.hpp"
using Andromeda::FSItems::Folder;

static a2fuse_operations a2fuse_ops;
static Debug sDebug("FuseAdapter");

namespace AndromedaFuse {

/*****************************************************/
/** Scope-managed fuse_args */
struct FuseArguments
{
    FuseArguments(): mFuseArgs(FUSE_ARGS_INIT(0,NULL))
    {
        sDebug << __func__ << "() fuse_opt_add_arg()"; sDebug.Info();

        int retval; if ((retval = fuse_opt_add_arg(&mFuseArgs, "andromeda-fuse")) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()1 failed: "+std::to_string(retval));
    };

    ~FuseArguments()
    { 
        sDebug << __func__ << "() fuse_opt_free_args()"; sDebug.Info();

        fuse_opt_free_args(&mFuseArgs); 
    };

    /** @param arg -o fuse argument */
    void AddArg(const std::string& arg)
    { 
        sDebug << __func__ << "(arg:" << arg << ")"; sDebug.Info(); int retval;

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
    sDebug << __func__ << "... fuse_daemonize()"; sDebug.Info();

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
        sDebug << __func__ << "() fuse_mount()"; sDebug.Info();
        
        mFuseChan = fuse_mount(mPath, &fargs.mFuseArgs);
        
        if (!mFuseChan) throw FuseAdapter::Exception("fuse_mount() failed");
    };

    void Unmount()
    {
        if (mFuseChan == nullptr) return;

        sDebug << __func__ << "() fuse_unmount()"; sDebug.Info();
        
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
        sDebug << __func__ << "() fuse_new()"; sDebug.Info();

        mFuse = fuse_new(mMount.mFuseChan, &(fargs.mFuseArgs), 
            &a2fuse_ops, sizeof(a2fuse_ops), static_cast<void*>(&adapter));
        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");
    };

    ~FuseContext()
    {
        mMount.Unmount(); 

        sDebug << __func__ << "() fuse_destroy()"; sDebug.Info();

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
        sDebug << __func__ << "() fuse_new()"; sDebug.Info();

        mFuse = fuse_new(&(fargs.mFuseArgs), 
            &a2fuse_ops, sizeof(a2fuse_ops), static_cast<void*>(&adapter));
        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");
    };

    ~FuseContext()
    {
        sDebug << __func__ << "() fuse_destroy()"; sDebug.Info();

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
        sDebug << __func__ << "() fuse_mount()"; sDebug.Info();

        int retval; if ((retval = fuse_mount(mContext.mFuse, path)) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_mount() failed: "+std::to_string(retval));
    };

    ~FuseMount()
    {
        sDebug << __func__ << "() fuse_unmount()"; sDebug.Info();
        
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
        sDebug << __func__ << "() fuse_set_signal_handlers()"; sDebug.Info();

        mFuseSession = fuse_get_session(context.mFuse);

        int retval; if ((retval = fuse_set_signal_handlers(mFuseSession)) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_set_signal_handlers() failed: "+std::to_string(retval));
    };

    ~FuseSignals()
    {
        sDebug << __func__ << "() fuse_remove_signal_handlers()"; sDebug.Info();

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
        sDebug << __func__ << "() fuse_loop()"; sDebug.Info();

        mAdapter.mFuseLoop = this; // register with adapter

        int retval; if ((retval = fuse_loop(context.mFuse)) < 0)
            throw FuseAdapter::Exception("fuse_loop() failed: "+std::to_string(retval));

        sDebug << __func__ << "() fuse_loop() returned!"; sDebug.Info();

        // TODO maybe translate some of the errnos around?
    }

    ~FuseLoop() { mAdapter.mFuseLoop = nullptr; }

    /** Flags the fuse session to terminate */
    void ExitLoop() 
    {
        sDebug << __func__ << "()"; sDebug.Info();

        fuse_exit(mContext.mFuse); 
    }

    FuseContext& mContext;
    FuseAdapter& mAdapter;
};

/*****************************************************/
FuseAdapter::FuseAdapter(Folder& root, const Options& options, RunMode runMode)
    : mRootFolder(root), mOptions(options)
{
    sDebug << __func__ << "(path:" << mOptions.mountPath << ")"; sDebug.Info();

    if (runMode == RunMode::THREAD)
    {
        mFuseThread = std::thread(&FuseAdapter::RunFuse, this, runMode);

        sDebug << __func__ << "()... waiting for init"; sDebug.Info();

        std::unique_lock<std::mutex> initLock(mInitMutex);
        while (!mInitialized) mInitCV.wait(initLock);

        sDebug << __func__ << "()... init complete!"; sDebug.Info();
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
    sDebug << __func__ << "()"; sDebug.Info();

    try
    {
        FuseArguments fuseArgs; 
        for (const std::string& fuseArg : mOptions.fuseArgs)
            fuseArgs.AddArg(fuseArg);

    #if LIBFUSE2
        FuseMount mount(fuseArgs, mOptions.mountPath.c_str());
        FuseContext context(mount, fuseArgs, *this);
    #else // !LIBFUSE2
        FuseContext context(fuseArgs, *this);
        FuseMount mount(context, mOptions.mountPath.c_str());
    #endif // LIBFUSE2
        
        if (runMode == RunMode::DAEMON) FuseDaemonize();

        std::unique_ptr<FuseSignals> signals; 
        if (runMode != RunMode::THREAD) 
            signals = std::make_unique<FuseSignals>(context);
        
        FuseLoop loop(context, *this); // blocks until done
    }
    catch (const Exception& ex)
    {
        sDebug << __func__ << "(error: " << ex.what() << ")"; sDebug.Error();

        mInitError = std::current_exception();
    }

    SignalInit(); // outside catch just in case fuse fails but doesn't throw
}

/*****************************************************/
void FuseAdapter::SignalInit()
{
    sDebug << __func__ << "()"; sDebug.Info();

    std::unique_lock<std::mutex> initLock(mInitMutex);
    mInitialized = true; mInitCV.notify_all();
}

/*****************************************************/
FuseAdapter::~FuseAdapter()
{
    sDebug << __func__ << "()"; sDebug.Info();
    
    if (mFuseLoop) 
        mFuseLoop->ExitLoop();

    sDebug << __func__ << "()... waiting"; sDebug.Info();

    if (mFuseThread.joinable())
        mFuseThread.join();

    sDebug << __func__ << "()... return!"; sDebug.Info();
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
