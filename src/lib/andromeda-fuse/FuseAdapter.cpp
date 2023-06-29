
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
#include "andromeda/SharedMutex.hpp"
using Andromeda::SharedLockW;
#include "andromeda/backend/CLIRunner.hpp"
using Andromeda::Backend::CLIRunner;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;

using UniqueLock = std::unique_lock<std::mutex>;

namespace AndromedaFuse {

/*****************************************************/
/** Scope-managed fuse_args */
struct FuseArguments
{
    FuseArguments(): mDebug(__func__,this), mFuseArgs(FUSE_ARGS_INIT(0,nullptr))
    {
        MDBG_INFO("() fuse_opt_add_arg()");

        const int retval { fuse_opt_add_arg(&mFuseArgs, "andromeda-fuse") };
        if (retval != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()1 failed",retval);
    };

    ~FuseArguments()
    { 
        MDBG_INFO("() fuse_opt_free_args()");

        fuse_opt_free_args(&mFuseArgs); 
    };
    DELETE_COPY(FuseArguments)
    DELETE_MOVE(FuseArguments)

    /** @param arg -o fuse argument */
    void AddArg(const std::string& arg)
    { 
        MDBG_INFO("(arg:" << arg << ")");

        { const int retval { fuse_opt_add_arg(&mFuseArgs, "-o") };
        if (retval != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()2 failed",retval); }

        { const int retval { fuse_opt_add_arg(&mFuseArgs, arg.c_str()) };
        if (retval != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()3 failed",retval); }
    };

    mutable Debug mDebug;
    /** fuse_args struct */
    struct fuse_args mFuseArgs;
};

#if LIBFUSE2

/*****************************************************/
/** fuse_mount (manual unmount, not scope-managed) */
struct FuseMount
{
    /** @param fargs FuseArguments reference
      * @param path filesystem path to mount */
    FuseMount(FuseArguments& fargs, const char* const path): 
        mDebug(__func__,this), mPath(path)
    {
        MDBG_INFO("() fuse_mount(path:" << path << ")");

        mFuseChan = fuse_mount(mPath, &fargs.mFuseArgs); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        if (!mFuseChan) throw FuseAdapter::Exception("fuse_mount() failed");
    };

    /** Unmount the FUSE session - MUST be called before destruct */
    void Unmount()
    {
        if (mFuseChan == nullptr) return;
        mFuseChan = nullptr;

        MDBG_INFO("() fuse_unmount()");
        fuse_unmount(mPath, mFuseChan); 
    };
    DELETE_COPY(FuseMount)
    DELETE_MOVE(FuseMount)

    mutable Debug mDebug;
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
        mDebug(__func__,this), mAdapter(adapter), mMount(mount)
    { 
        MDBG_INFO("() fuse_new()");

        mFuse = fuse_new(mMount.mFuseChan, &(fargs.mFuseArgs), // NOLINT(cppcoreguidelines-prefer-member-initializer)
            &mOps, sizeof(mOps), static_cast<void*>(&adapter));
        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");

        mAdapter.mFuseContext = this; // register with adapter
    };

    ~FuseContext()
    {
        mAdapter.mFuseContext = nullptr;

        MDBG_INFO("()");
        mMount.Unmount(); 

        MDBG_INFO("... fuse_destroy()");
        fuse_destroy(mFuse);
    };
    DELETE_COPY(FuseContext)
    DELETE_MOVE(FuseContext)

    /** Exit and unmount the FUSE session */
    void TriggerUnmount()
    {
        MDBG_INFO("() fuse_exit()");
        fuse_exit(mFuse); // flag loop to stop

        // fuse_exit does not interrupt the loop, so to prevent it hanging until the next FS operation
        // we will send off a umount command... ugly, but see https://github.com/winfsp/cgofuse/issues/6#issuecomment-298185815
        // fuse_unmount() is not valid on this thread, and can't use unmount() library call as that requires superuser
        // ... doing this as a command rather than C call so we get the setuid elevation of umount
        
        #if APPLE
            // OSX hangs our whole process (not just this thread) 60 seconds if we start a
            // background process but fortunately it allows unmount() without being a superuser
            MDBG_INFO("... calling unmount(2)");
            unmount(mMount.mPath, MNT_FORCE);
            MDBG_INFO("... unmount returned");
        #else
            mAdapter.TrySystemUnmount(mMount.mPath);
        #endif
    }

    mutable Debug mDebug;
    FuseAdapter& mAdapter;
    /** Fuse mount reference */
    FuseMount& mMount;
    /** fuse operations struct */
    a2fuse_operations mOps;
    /** Fuse context pointer */
    struct fuse* mFuse;
};

#else // !LIBFUSE2

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param fargs FuseArguments reference */
    FuseContext(FuseAdapter& adapter, FuseArguments& fargs): mDebug(__func__,this)
    {
        MDBG_INFO("() fuse_new()");

        mFuse = fuse_new(&(fargs.mFuseArgs), // NOLINT(cppcoreguidelines-prefer-member-initializer)
            &mOps, sizeof(mOps), static_cast<void*>(&adapter));

        if (!mFuse) throw FuseAdapter::Exception("fuse_new() failed");
    };

    ~FuseContext()
    {
        MDBG_INFO("() fuse_destroy()");
        fuse_destroy(mFuse);
    };
    DELETE_COPY(FuseContext)
    DELETE_MOVE(FuseContext)

    mutable Debug mDebug;
    /** fuse operations struct */
    a2fuse_operations mOps;
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
        mDebug(__func__,this), mAdapter(adapter), mContext(context), mPath(path)
    {
        MDBG_INFO("() fuse_mount(path:" << path << ")");

        const int retval { fuse_mount(mContext.mFuse, path) };
        if (retval != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_mount() failed",retval);
        mAdapter.mFuseMount = this; // register with adapter
    };

    /** Exit and unmount the FUSE session */
    void TriggerUnmount() const
    {
        MDBG_INFO("() fuse_exit()");
        fuse_exit(mContext.mFuse);// flag loop to stop

        // fuse_exit does not interrupt the loop (except WinFSP), so to prevent it hanging until the next FS operation
        // we will send off a umount command... ugly, but see https://github.com/winfsp/cgofuse/issues/6#issuecomment-298185815
        // fuse_unmount() is not valid on this thread, and can't use unmount() library call as that requires superuser
        // ... doing this as a command rather than C call so we get the setuid elevation of umount
        #if !WIN32
            mAdapter.TrySystemUnmount(mPath);
        #endif
    }

    ~FuseMount() 
    {
        mAdapter.mFuseMount = nullptr;

        MDBG_INFO("() fuse_unmount()");
        fuse_unmount(mContext.mFuse);
    }
    DELETE_COPY(FuseMount)
    DELETE_MOVE(FuseMount)

    mutable Debug mDebug;
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
    explicit FuseSignals(FuseContext& context) : mDebug(__func__,this)
    { 
        MDBG_INFO("() fuse_set_signal_handlers()");

        mFuseSession = fuse_get_session(context.mFuse); // NOLINT(cppcoreguidelines-prefer-member-initializer)

        const int retval { fuse_set_signal_handlers(mFuseSession) };
        if (retval != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_set_signal_handlers() failed",retval);
    };

    ~FuseSignals()
    {
        MDBG_INFO("() fuse_remove_signal_handlers()");

        fuse_remove_signal_handlers(mFuseSession); 
    }
    DELETE_COPY(FuseSignals)
    DELETE_MOVE(FuseSignals)

    mutable Debug mDebug;
    /** fuse_session pointer */
    struct fuse_session* mFuseSession;
};

/*****************************************************/
FuseAdapter::FuseAdapter(const std::string& mountPath, Folder& root, const FuseOptions& options) :
    mDebug(__func__,this), mMountPath(mountPath), mOptions(options),
    mRootFolder(root.TryLockScope()) // assume valid
{
    MDBG_INFO("(path:" << mMountPath << ")");
}

/*****************************************************/
void FuseAdapter::StartFuse(FuseAdapter::RunMode runMode, const FuseAdapter::ForkFunc& forkFunc)
{
    if (mFuseThread.joinable())
        mFuseThread.join();

    if (runMode == RunMode::THREAD)
    {
        mFuseThread = std::thread(&FuseAdapter::FuseMain, this,
        // do not register signal handlers, do not daemonize
            false, false, std::ref(forkFunc));

        MDBG_INFO("... waiting for init");

        UniqueLock initLock(mInitMutex);
        while (!mInitialized) mInitCV.wait(initLock);

        MDBG_INFO("... init complete!");
    }
    else FuseMain(true, (runMode == RunMode::DAEMON), forkFunc); // blocking

    if (mInitError)
    {
        if (runMode == RunMode::THREAD)
            mFuseThread.join();

        std::rethrow_exception(mInitError);
    }
}

/*****************************************************/
void FuseAdapter::FuseMain(bool regSignals, bool daemonize, const FuseAdapter::ForkFunc& forkFunc)
{
    MDBG_INFO("()");

    try
    {
        FuseArguments fuseArgs; 
        fuseArgs.AddArg("default_permissions");
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
        const FuseMount mount(*this, context, mMountPath.c_str());
    #endif // LIBFUSE2

        if (daemonize)
        {
            MDBG_INFO("... fuse_daemonize()");

            const int retval { fuse_daemonize(0) };
            if (retval != FUSE_SUCCESS)
                throw FuseAdapter::Exception("fuse_daemonize() failed",retval);

            if (forkFunc) forkFunc();
        }
        
        const std::unique_ptr<FuseSignals> signalsPtr {
            regSignals ? std::make_unique<FuseSignals>(context) : nullptr };
        
        MDBG_INFO("() fuse_loop()");

        { // retval scope
            int retval = -1;
            if (mOptions.enableThreading)
            {
            #if LIBFUSE2
                retval = fuse_loop_mt(context.mFuse);
            #else // !LIBFUSE2
                struct fuse_loop_config loop_config { }; // zero
                loop_config.max_idle_threads = mOptions.maxIdleThreads;
                retval = fuse_loop_mt(context.mFuse, &loop_config);
            #endif // LIBFUSE2
            }
            else retval = fuse_loop(context.mFuse);

            if (retval < 0)
                throw FuseAdapter::Exception("fuse_loop() failed",retval); 
        }

        MDBG_INFO("() fuse_loop() returned!");
    }
    catch (const Exception& ex)
    {
        MDBG_ERROR("... error: " << ex.what());
        mInitError = std::current_exception();
    }

    SignalInit(); // just in case fuse fails but doesn't throw
}

/*****************************************************/
void FuseAdapter::SignalInit()
{
    MDBG_INFO("()");

    const UniqueLock initLock(mInitMutex);
    mInitialized = true; 
    mInitCV.notify_all();
}

/*****************************************************/
FuseAdapter::~FuseAdapter()
{
    MDBG_INFO("()");
    
#if LIBFUSE2
    if (mFuseContext) 
        mFuseContext->TriggerUnmount();
#else // !LIBFUSE2
    if (mFuseMount) 
        mFuseMount->TriggerUnmount();
#endif // LIBFUSE2

    if (mFuseThread.joinable())
    {
        MDBG_INFO("... waiting");
        mFuseThread.join();
    }

    const SharedLockW rootLock { mRootFolder->GetWriteLock() };
    mRootFolder->FlushCache(rootLock, true); // dump caches

    MDBG_INFO("... return!");
}

#if !WIN32
/*****************************************************/
void FuseAdapter::TrySystemUnmount(const std::string& path)
{
     // failure is okay, just an optimization for fuse_exit
    try {
        MDBG_INFO("(path:" << path << ")");
        CLIRunner::ArgList args { "umount", path };
        int retval { CLIRunner::RunPosixCommand(args) };
        if (retval != 0) MDBG_ERROR("... system umount returned:" << retval);
    }
    catch (const CLIRunner::CmdException& ex) {
        MDBG_ERROR("... system umount threw: " << ex.what());
    }
}
#endif // !WIN32

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
