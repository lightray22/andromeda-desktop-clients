
#include <iostream>

#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>

#include "FuseWrapper.hpp"
#include "Backend.hpp"
#include "filesystem/BaseFolder.hpp"
#include "filesystem/Item.hpp"

static Debug debug("FuseWrapper");
static BaseFolder* rootPtr = nullptr;

static int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
static int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);

static struct fuse_operations fuse_ops = {
    .getattr = fuse_getattr,
    .readdir = fuse_readdir
};

constexpr int SUCCESS = 0;

/*****************************************************/
void FuseWrapper::ShowHelpText()
{
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
        debug << __func__ << "() fuse_opt_free_args()"; debug.Info();

        fuse_opt_free_args(&args); 
    };
    void AddArg(const std::string& arg)
    { 
        debug << __func__ << "(arg:" << arg << ")"; debug.Info();

        if (fuse_opt_add_arg(&args, "-o") != SUCCESS)
            throw FuseWrapper::Exception("fuse_opt_add_arg()2 failed");

        if (fuse_opt_add_arg(&args, arg.c_str()) != SUCCESS)
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
        debug << __func__ << "() fuse_new()"; debug.Info();

        fuse = fuse_new(&(opts.args), &fuse_ops, sizeof(fuse_ops), (void*)nullptr);

        if (!fuse) throw FuseWrapper::Exception("fuse_new() failed");
    };
    ~FuseContext()
    { 
        debug << __func__ << "() fuse_destroy()"; debug.Info();

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
        debug << __func__ << "() fuse_mount()"; debug.Info();

        if (fuse_mount(fuse, path) != SUCCESS) 
            throw FuseWrapper::Exception("fuse_mount() failed");
    };
    ~FuseMount()
    { 
        debug << __func__ << "() fuse_unmount()"; debug.Info();

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
        debug << __func__ << "() fuse_set_signal_handlers()"; debug.Info();

        session = fuse_get_session(context.fuse);

        if (fuse_set_signal_handlers(session) != SUCCESS)
            throw FuseWrapper::Exception("fuse_set_signal_handlers() failed");
    };
    ~FuseSignals()
    {
        debug << __func__ << "() fuse_remove_signal_handlers()"; debug.Info();

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
        debug << __func__ << "() fuse_loop()"; debug.Info();

        int retval = fuse_loop(context.fuse);

        debug << __func__ << "() fuse_loop() returned! retval:" << std::to_string(retval); debug.Info();
    }
};

/*****************************************************/
void FuseWrapper::Start(BaseFolder& root, const Options& options)
{
    rootPtr = &root; std::string mountPath(options.GetMountPath());    
    
    debug << __func__ << "(path:" << mountPath << ")"; debug.Info();

    // TODO add multithreading option: fuse_loop_mt() and fuse_loop_config

    // TODO set up FUSE logging module?

    // TODO see fuse_conn_info ... capability flags?
    // fuse_apply_conn_info_opts() : fuse_common.h
    // fuse_parse_conn_info_opts() from command line
    
    FuseOptions opts; for (const std::string& opt : options.GetFuseOptions()) opts.AddArg(opt);

    FuseContext context(opts); 
    FuseMount mount(context, mountPath.c_str());

    debug << __func__ << "... fuse_daemonize()"; debug.Info();
    if (fuse_daemonize(static_cast<int>(Debug::GetLevel())) != SUCCESS)
        throw FuseWrapper::Exception("fuse_daemonize() failed");
    
    FuseSignals signals(context); 
    FuseLoop loop(context);
}

/*****************************************************/
/*****************************************************/
/*****************************************************/

int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
    if (path[0] == '/') path++;

    debug << __func__ << "(path:" << path << ")"; debug.Info();

    try
    {
        Item& item = rootPtr->GetItemByPath(path);

        switch (item.GetType())
        {
            case Item::Type::FILE: stbuf->st_mode = S_IFREG; break;
            case Item::Type::FOLDER: stbuf->st_mode = S_IFDIR; break;
        }

        stbuf->st_size = static_cast<off_t>(item.GetSize());

        stbuf->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO; 
        // TODO mount permissions? what about uid/gid

        // TODO set block size...? explore kernel caching settings

        stbuf->st_atime = static_cast<time_t>(item.GetAccessed());
        stbuf->st_ctime = static_cast<time_t>(item.GetCreated());
        stbuf->st_mtime = static_cast<time_t>(item.GetModified());

        return SUCCESS;
    }
    catch (const Backend::APINotFoundException& e)
    {
        debug << __func__ << "..." << e.what(); debug.Details(); return ENOENT;
    }
    catch (const Utilities::Exception& e)
    {
        debug << __func__ << "..." << e.what(); debug.Error(); return EIO;
    }
}

/*****************************************************/
int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
    if (path[0] == '/') path++;

    debug << __func__ << "(path:" << path << ")"; debug.Info();

    try
    {
        Item& item = rootPtr->GetItemByPath(path);

        // TODO implement me

        return SUCCESS;
    }
    catch (const Backend::APINotFoundException& e)
    {
        debug << __func__ << "..." << e.what(); debug.Details(); return ENOENT;
    }
    catch (const Utilities::Exception& e)
    {
        debug << __func__ << "..." << e.what(); debug.Error(); return EIO;
    }
}
