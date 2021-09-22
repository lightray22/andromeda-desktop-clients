
#include <iostream>
#include <functional>

#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>

#include "FuseWrapper.hpp"
#include "Backend.hpp"
#include "filesystem/Folder.hpp"
#include "filesystem/Item.hpp"

static Debug debug("FuseWrapper");
static Folder* rootPtr = nullptr;

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
void FuseWrapper::Start(Folder& root, const Options& options)
{
    rootPtr = &root; std::string mountPath(options.GetMountPath());    
    
    debug << __func__ << "(path:" << mountPath << ")"; debug.Info();

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

/*****************************************************/
int standardTry(std::function<int()> func)
{
    try
    {
        return func();
    }
    catch (const Folder::NotFileException& e)
    {
        debug << __func__ << "..." << e.what(); debug.Details(); return -EISDIR;
    }
    catch (const Folder::NotFolderException& e)
    {
        debug << __func__ << "..." << e.what(); debug.Details(); return -ENOTDIR;
    }
    catch (const Backend::NotFoundException& e)  
    {
        debug << __func__ << "..." << e.what(); debug.Details(); return -ENOENT;
    }
    catch (const Utilities::Exception& e)
    {
        debug << __func__ << "..." << e.what(); debug.Error(); return -EIO;
    }
}

/*****************************************************/
void item_stat(const Item& item, struct stat* stbuf)
{
    switch (item.GetType())
    {
        case Item::Type::FILE: stbuf->st_mode = S_IFREG; break;
        case Item::Type::FOLDER: stbuf->st_mode = S_IFDIR; break;
    }

    stbuf->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;

    stbuf->st_size = static_cast<off_t>(item.GetSize());

    stbuf->st_ctime = static_cast<time_t>(item.GetCreated());

    stbuf->st_atime = static_cast<time_t>(item.GetAccessed());
    if (!stbuf->st_atime) stbuf->st_atime = stbuf->st_ctime;

    stbuf->st_mtime = static_cast<time_t>(item.GetModified());
    if (!stbuf->st_mtime) stbuf->st_mtime = stbuf->st_ctime;
}

/*****************************************************/
int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
{
    if (path[0] == '/') path++;

    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        item_stat(rootPtr->GetItemByPath(path), stbuf);

        debug << __func__ << "... RETURN"; debug.Details(); return SUCCESS;
    });
}

/*****************************************************/
int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
{
    if (path[0] == '/') path++;

    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        Folder& folder(rootPtr->GetFolderByPath(path));
        const Folder::ItemMap& items(folder.GetItems());

        debug << __func__ << "... #items:" << std::to_string(items.size()); debug.Details();

        for (const Folder::ItemMap::value_type& pair : items)
        {
            const std::unique_ptr<Item>& item = pair.second;

            struct stat stbuf; item_stat(*item, &stbuf);

            if (filler(buf, item->GetName().c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS) != SUCCESS) 
            {
                debug << __func__ << "... filler() failed"; debug.Error(); return -EIO;
            }
        }

        debug << __func__ << "... RETURN"; debug.Details(); return SUCCESS;
    });
}
