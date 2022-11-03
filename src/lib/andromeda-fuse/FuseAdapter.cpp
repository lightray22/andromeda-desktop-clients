
#include <iostream>

#include "libfuse_Includes.h"
#include "FuseAdapter.hpp"

static Debug debug("FuseAdapter");

extern struct fuse_operations a2fuse_ops;

/*****************************************************/
/** Scope-managed fuse_args */
struct FuseArguments
{
    FuseArguments(): args(FUSE_ARGS_INIT(0,NULL))
    {
        if (fuse_opt_add_arg(&args, "andromeda-fuse"))
            throw FuseAdapter::Exception("fuse_opt_add_arg()1 failed");
    };
    ~FuseArguments()
    { 
        debug << __func__ << "() fuse_opt_free_args()"; debug.Info();

        fuse_opt_free_args(&args); 
    };
    /** @param arg -o fuse argument */
    void AddArg(const std::string& arg)
    { 
        debug << __func__ << "(arg:" << arg << ")"; debug.Info();

        if (fuse_opt_add_arg(&args, "-o") != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()2 failed");

        if (fuse_opt_add_arg(&args, arg.c_str()) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_opt_add_arg()3 failed");
    };
    /** fuse_args struct */
    struct fuse_args args;
};

#if USE_FUSE2

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    /** @param fargs FuseArguments reference
     * @param path filesystem path to mount */
    FuseMount(FuseArguments& fargs, const char* path): path(path)
    {
        debug << __func__ << "() fuse_mount()"; debug.Info();
        
        chan = fuse_mount(path, &fargs.args);
        
        if (!chan) throw FuseAdapter::Exception("fuse_mount() failed");
    };
    void Unmount()
    {
        if (chan == nullptr) return;

        debug << __func__ << "() fuse_unmount()"; debug.Info();
        
        fuse_unmount(path.c_str(), chan); chan = nullptr;
    };
    ~FuseMount(){ Unmount(); };
    
    /** mounted path */
    const std::string path;
    /** fuse_chan pointer */
    struct fuse_chan* chan;
};

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param mount FuseMount reference 
     * @param fargs FuseArguments reference */
    FuseContext(FuseMount& mount, FuseArguments& fargs, FuseAdapter& adapter): mount(mount)
    { 
        debug << __func__ << "() fuse_new()"; debug.Info();

        fuse = fuse_new(mount.chan, &(fargs.args), &a2fuse_ops, sizeof(a2fuse_ops), (void*)&adapter);

        if (!fuse) throw FuseAdapter::Exception("fuse_new() failed");
    };
    ~FuseContext()
    {
        mount.Unmount(); 

        debug << __func__ << "() fuse_destroy()"; debug.Info();

        fuse_destroy(fuse);
    };
    /** Fuse context pointer */
    struct fuse* fuse;
    /** Fuse mount reference */
    FuseMount& mount;
};

#else

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    /** @param fargs FuseArguments reference */
    FuseContext(FuseArguments& fargs, FuseAdapter& adapter)
    {
        debug << __func__ << "() fuse_new()"; debug.Info();
        
        fuse = fuse_new(&(fargs.args), &a2fuse_ops, sizeof(a2fuse_ops), (void*)&adapter);
        
        if (!fuse) throw FuseAdapter::Exception("fuse_new() failed");
    };
    ~FuseContext()
    {
        debug << __func__ << "() fuse_destroy()"; debug.Info();
        
        fuse_destroy(fuse);
    };
    /** Fuse context pointer */
    struct fuse* fuse;
};

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    /** @param context FuseContext reference
     * @param path filesystem path to mount */
    FuseMount(FuseContext& context, const char* path): fuse(context.fuse)
    {
        debug << __func__ << "() fuse_mount()"; debug.Info();

        if (fuse_mount(fuse, path) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_mount() failed");
    };
    ~FuseMount()
    {
        debug << __func__ << "() fuse_unmount()"; debug.Info();
        
        fuse_unmount(fuse);
    }
    /** Fuse context pointer */
    struct fuse* fuse;
};

#endif

/*****************************************************/
/** Scope-managed fuse_set/remove_signal_handlers */
struct FuseSignals
{
    /** @param context FuseContext reference */
    explicit FuseSignals(FuseContext& context)
    { 
        debug << __func__ << "() fuse_set_signal_handlers()"; debug.Info();

        session = fuse_get_session(context.fuse);

        if (fuse_set_signal_handlers(session) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_set_signal_handlers() failed");
    };
    ~FuseSignals()
    {
        debug << __func__ << "() fuse_remove_signal_handlers()"; debug.Info();

        fuse_remove_signal_handlers(session); 
    }
    /** fuse_session pointer */
    struct fuse_session* session;
};

/*****************************************************/
/** Scope-managed fuse event loop - blocks */
struct FuseLoop
{
    /** @param context FUSE context reference */
    explicit FuseLoop(FuseContext& context)
    {
        debug << __func__ << "() fuse_loop()"; debug.Info();

        int retval = fuse_loop(context.fuse);

        debug << __func__ << "() fuse_loop() returned! retval:" << retval; debug.Info();
    }
};

/*****************************************************/
FuseAdapter::FuseAdapter(Folder& root, const Options& options, bool daemonize):
    rootFolder(root), options(options)
{
    debug << __func__ << "(path:" << options.mountPath << ")"; debug.Info();

    FuseArguments opts; for (const std::string& opt : options.fuseArgs) opts.AddArg(opt);

#if USE_FUSE2
    FuseMount mount(opts, options.mountPath.c_str());
    FuseContext context(mount, opts, *this);
#else
    FuseContext context(opts, *this);
    FuseMount mount(context, options.mountPath.c_str());
#endif
    
    if (daemonize)
    {
        debug << __func__ << "... fuse_daemonize()"; debug.Info();
        if (fuse_daemonize(static_cast<bool>(Debug::GetLevel())) != FUSE_SUCCESS)
            throw FuseAdapter::Exception("fuse_daemonize() failed");
    }

    FuseSignals signals(context); 
    FuseLoop loop(context);
}

/*****************************************************/
void FuseAdapter::ShowVersionText()
{
    std::cout << "libfuse version: " << fuse_version()
#if !USE_FUSE2
        << " (" << fuse_pkgversion() << ")"
#endif
        << std::endl;

#if !USE_FUSE2
    fuse_lowlevel_version();
#endif
}
