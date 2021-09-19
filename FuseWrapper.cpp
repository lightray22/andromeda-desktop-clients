
#include <iostream>

#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>

#include "FuseWrapper.hpp"
#include "filesystem/BaseFolder.hpp"

static Debug debug("FuseWrapper");
static BaseFolder* rootPtr = nullptr;

static int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
static int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);

static struct fuse_operations fuse_ops = {
    .getattr = fuse_getattr,
    .readdir = fuse_readdir
};

/*****************************************************/
void FuseWrapper::ShowHelpText()
{
    // TODO don't show cmdline_help ? do lowlevel_help or lib_help even apply?

    std::cout << "fuse_cmdline_help()" << std::endl; fuse_cmdline_help(); std::cout << std::endl;
    std::cout << "fuse_lowlevel_help()" << std::endl; fuse_lowlevel_help(); std::cout << std::endl;
    std::cout << "fuse_lib_help()" << std::endl; fuse_lib_help(0); std::cout << std::endl;
}

/*****************************************************/
void FuseWrapper::ShowVersionText()
{
    std::cout << "libfuse version: " << 
        std::to_string(fuse_version()) << 
        " (" << fuse_pkgversion() << ")" << std::endl;
    
    fuse_lowlevel_version();
}

/*****************************************************/
/** Scope-managed fuse_args */
struct FuseOptions
{
    FuseOptions():args(FUSE_ARGS_INIT(0,NULL))
    { 
        if (fuse_opt_add_arg(&args, "andromeda-fuse"))
            throw FuseWrapper::Exception("fuse_opt_add_arg()1 failed");
    };
    ~FuseOptions()
    { 
        debug << __func__ << "() fuse_opt_free_args()"; debug.Out();

        fuse_opt_free_args(&args); 
    };
    void AddArg(const std::string& arg)
    { 
        debug << __func__ << "(arg:" << arg << ")"; debug.Out();

        if (fuse_opt_add_arg(&args, "-o"))
            throw FuseWrapper::Exception("fuse_opt_add_arg()2 failed");

        if (fuse_opt_add_arg(&args, arg.c_str()))
            throw FuseWrapper::Exception("fuse_opt_add_arg()3 failed");
    };
    struct fuse_args args;
};

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    FuseContext(FuseOptions& opts)
    { 
        debug << __func__ << "() fuse_new()"; debug.Out();

        fuse = fuse_new(&(opts.args), &fuse_ops, sizeof(fuse_ops), (void*)nullptr);

        if (!fuse) throw FuseWrapper::Exception("fuse_new() failed");
    };
    ~FuseContext()
    { 
        debug << __func__ << "() fuse_destroy()"; debug.Out();

        fuse_destroy(fuse); 
    };
    struct fuse* fuse;
};

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    FuseMount(FuseContext& context, const char* path):fuse(context.fuse)
    {
        debug << __func__ << "() fuse_mount()"; debug.Out();

        if (fuse_mount(fuse, path)) throw FuseWrapper::Exception("fuse_mount() failed");
    };
    ~FuseMount()
    { 
        debug << __func__ << "() fuse_unmount()"; debug.Out();

        fuse_unmount(fuse); 
    };
    struct fuse* fuse;
};

/*****************************************************/
/** Scope-managed fuse_set/remove_signal_handlers */
struct FuseSignals
{
    FuseSignals(FuseContext& context)
    { 
        debug << __func__ << "() fuse_set_signal_handlers()"; debug.Out();

        session = fuse_get_session(context.fuse);

        if (fuse_set_signal_handlers(session))
            throw FuseWrapper::Exception("fuse_set_signal_handlers() failed");
    };
    ~FuseSignals()
    {
        debug << __func__ << "() fuse_remove_signal_handlers()"; debug.Out();

        fuse_remove_signal_handlers(session); 
    }
    struct fuse_session* session;
};

/*****************************************************/
/** Scope-managed fuse_loop (print on return) */
struct FuseLoop
{
    FuseLoop(FuseContext& context)
    {
        debug << __func__ << "() fuse_loop()"; debug.Out();

        fuse_loop(context.fuse);

        debug << __func__ << "() fuse_loop() returned!"; debug.Out();
    }
};

/*****************************************************/
void FuseWrapper::Start(BaseFolder& root, const Options& options)
{
    rootPtr = &root; std::string mountPath(options.GetMountPath());    
    
    debug << __func__ << "(path:" << mountPath << ")"; debug.Out();

    // TODO add multithreading option: fuse_loop_mt() and fuse_loop_config

    // TODO set up FUSE logging module?

    // TODO see fuse_conn_info ... capability flags?
    // fuse_apply_conn_info_opts() : fuse_common.h
    // fuse_parse_conn_info_opts() from command line
    
    FuseOptions opts; for (const std::string& opt : options.GetFuseOptions()) opts.AddArg(opt);

    FuseContext context(opts); 
    FuseMount mount(context, mountPath.c_str());

    debug << __func__ << "... fuse_daemonize()"; debug.Out();
    if (fuse_daemonize(static_cast<int>(Debug::GetLevel())))
        throw FuseWrapper::Exception("fuse_daemonize() failed");
    
    FuseSignals signals(context); 
    FuseLoop loop(context);
}

/*****************************************************/
/*****************************************************/
/*****************************************************/

int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
    debug << __func__ << "(path:" << path << ")"; debug.Out();

    // TODO rootPtr->

    return 0;
}

int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
    debug << __func__ << "(path:" << path << ")"; debug.Out();

    // TODO rootPtr->

    return 0;
}
