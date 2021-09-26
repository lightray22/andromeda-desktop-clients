
#include <iostream>
#include <functional>

#ifdef __APPLE__
#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#else
#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#endif

#include "FuseWrapper.hpp"
#include "Backend.hpp"
#include "filesystem/Item.hpp"
#include "filesystem/File.hpp"
#include "filesystem/Folder.hpp"

static Debug debug("FuseWrapper");
static Folder* rootPtr = nullptr;

static int fuse_statfs(const char *path, struct statvfs* buf);
static int fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi);
static int fuse_mkdir(const char* path, mode_t mode);
static int fuse_unlink(const char* path);
static int fuse_rmdir(const char* path);
static int fuse_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi);
static int fuse_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi);
static int fuse_fsync(const char* path, int datasync, struct fuse_file_info* fi);

#ifdef __APPLE__
static int fuse_getattr(const char* path, struct stat* stbuf);
static int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
static int fuse_rename(const char* oldpath, const char* newpath);
static int fuse_truncate(const char* path, off_t size);
#else
static int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
static int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
static int fuse_rename(const char* oldpath, const char* newpath, unsigned int flags);
static int fuse_truncate(const char* path, off_t size, struct fuse_file_info* fi);
#endif

/* TODO
flush??
copy_file_range?
fsyncdir?
chmod/chown??? stub
fallocate? (vs. truncate?)
*/

static struct fuse_operations fuse_ops = {
    .getattr = fuse_getattr,
    .mkdir = fuse_mkdir,
    .unlink = fuse_unlink,
    .rmdir = fuse_rmdir,
    .rename = fuse_rename,
    .truncate = fuse_truncate,
    .read = fuse_read,
    .write = fuse_write,
    .statfs = fuse_statfs,
    .fsync = fuse_fsync,
    .readdir = fuse_readdir,
    .create = fuse_create
};

constexpr int SUCCESS = 0;

/*****************************************************/
void FuseWrapper::ShowHelpText()
{
#ifndef __APPLE__
    std::cout << "fuse_lib_help()" << std::endl; fuse_lib_help(0); std::cout << std::endl;
#endif
}

/*****************************************************/
void FuseWrapper::ShowVersionText()
{
    std::cout << "libfuse version: "
        << std::to_string(fuse_version())
#ifndef __APPLE__
        << " (" << fuse_pkgversion() << ")"
#endif
        << std::endl;

#ifndef __APPLE__
    fuse_lowlevel_version();
#endif
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

#ifdef __APPLE__

/*****************************************************/
/** Scope-managed fuse_mount/fuse_unmount */
struct FuseMount
{
    FuseMount(FuseOptions& options, const char* path):path(path)
    {
        debug << __func__ << "() fuse_mount()"; debug.Info();
        
        chan = fuse_mount(path, &options.args); mounted = true;
        
        if (!chan) throw FuseWrapper::Exception("fuse_mount() failed");
    };
    void Unmount()
    {
        debug << __func__ << "() fuse_unmount()"; debug.Info();
        
        if (mounted) fuse_unmount(path.c_str(), chan);
    }
    
    bool mounted;
    const std::string path;
    struct fuse_chan* chan;
};

/*****************************************************/
/** Scope-managed fuse_new/fuse_destroy */
struct FuseContext
{
    FuseContext(FuseMount& mount, FuseOptions& opts):mount(mount)
    { 
        debug << __func__ << "() fuse_new()"; debug.Info();

        fuse = fuse_new(mount.chan, &(opts.args), &fuse_ops, sizeof(fuse_ops), (void*)nullptr);

        if (!fuse) throw FuseWrapper::Exception("fuse_new() failed");
    };
    ~FuseContext()
    { 
        debug << __func__ << "() fuse_destroy()"; debug.Info();

        mount.Unmount(); fuse_destroy(fuse);
    };
    struct fuse* fuse;
    FuseMount& mount;
};

#else

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
    }
    struct fuse* fuse;
};

#endif

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

#ifdef __APPLE__
    FuseMount mount(opts, mountPath.c_str());
    FuseContext context(mount, opts);
#else
    FuseContext context(opts); 
    FuseMount mount(context, mountPath.c_str());
#endif
    
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
    catch (const Folder::DuplicateItemException& e)
    {
        debug << __func__ << "..." << e.what(); debug.Details(); return -EEXIST;
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
int fuse_statfs(const char *path, struct statvfs* buf)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    buf->f_namemax = 255;
    
    // The 'f_favail', 'f_fsid' and 'f_flag' fields are ignored 
//           struct statvfs {
//           unsigned long  f_bsize;    /* Filesystem block size */
//           unsigned long  f_frsize;   /* Fragment size */
//           fsblkcnt_t     f_blocks;   /* Size of fs in f_frsize units */
//           fsblkcnt_t     f_bfree;    /* Number of free blocks */
//           fsblkcnt_t     f_bavail;   /* Number of free blocks for
//                                         unprivileged users */
//           fsfilcnt_t     f_files;    /* Number of inodes */
//           fsfilcnt_t     f_ffree;    /* Number of free inodes */
//    ///    fsfilcnt_t     f_favail;   /* Number of free inodes for
//                                         unprivileged users */
//    ///    unsigned long  f_fsid;     /* Filesystem ID */
//    ///    unsigned long  f_flag;     /* Mount flags */
//           unsigned long  f_namemax;  /* Maximum filename length */

// files getlimits & files getlimits --filesystem to calculate free space?
// then the root folder knows the total space used...?
// DO actually need the path as the answer would be different for each SuperRoot filesystem
// this could be a call into a filesystem object... superRoot would need to extend Filesystem

    return -EIO; // TODO
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

    stbuf->st_mtime = static_cast<time_t>(item.GetModified());
    if (!stbuf->st_mtime) stbuf->st_mtime = stbuf->st_ctime;

    stbuf->st_atime = static_cast<time_t>(item.GetAccessed());
    if (!stbuf->st_atime) stbuf->st_atime = stbuf->st_ctime;
}

/*****************************************************/
#ifdef __APPLE__
int fuse_getattr(const char* path, struct stat* stbuf)
#else
int fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
#endif
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        item_stat(rootPtr->GetItemByPath(path), stbuf);

        debug << __func__ << "... RETURN SUCCESS"; debug.Details(); return SUCCESS;
    });
}

/*****************************************************/
#ifdef __APPLE__
int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
#else
int fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
#endif
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        const Folder::ItemMap& items(rootPtr->GetFolderByPath(path).GetItems());

        debug << __func__ << "... #items:" << std::to_string(items.size()); debug.Details();

        for (const Folder::ItemMap::value_type& pair : items)
        {
            const std::unique_ptr<Item>& item = pair.second;

            debug << __func__ << "... subitem: " << item->GetName(); debug.Details();
            
#ifdef __APPLE__
            int retval = filler(buf, item->GetName().c_str(), NULL, 0);
#else
            int retval; if (flags & FUSE_READDIR_PLUS)
            {
                struct stat stbuf; item_stat(*item, &stbuf);

                retval = filler(buf, item->GetName().c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS);
            }
            else retval = filler(buf, item->GetName().c_str(), NULL, 0, (fuse_fill_dir_flags)0);
#endif
            
            if (retval != SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        debug << __func__ << "... RETURN SUCCESS"; debug.Details(); return SUCCESS;
    });
}

/*****************************************************/
int fuse_create(const char* fullpath, mode_t mode, struct fuse_file_info* fi)
{
    const auto [path, name] = Utilities::split(fullpath,"/",true);
    
    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry([path=path,name=name]()->int
    {
        Folder& parent(rootPtr->GetFolderByPath(path));

        parent.CreateFile(name); return SUCCESS;
    });
}

/*****************************************************/
int fuse_mkdir(const char* fullpath, mode_t mode)
{
    const auto [path, name] = Utilities::split(fullpath,"/",true);
    
    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry([path=path,name=name]()->int
    {
        Folder& parent(rootPtr->GetFolderByPath(path));

        parent.CreateFolder(name); return SUCCESS;
    });
}

/*****************************************************/
int fuse_unlink(const char* path)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        rootPtr->GetFileByPath(path).Delete(); return SUCCESS;
    });
}

/*****************************************************/
int fuse_rmdir(const char* path)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        rootPtr->GetFolderByPath(path).Delete(); return SUCCESS;
    });
}

/*****************************************************/
#ifdef __APPLE__
int fuse_rename(const char* oldpath, const char* newpath)
#else
int fuse_rename(const char* oldpath, const char* newpath, unsigned int flags)
#endif
{
    oldpath++; newpath++; debug << __func__ << "(oldpath:" << oldpath << " newpath:" << newpath << ")"; debug.Info();

    return standardTry([&]()->int
    {
        return -EIO; // TODO
    });
}

/*****************************************************/
int fuse_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << " size:" << std::to_string(size) 
        << " offset:" << std::to_string(off) << ")"; debug.Info();

    return standardTry([&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        return -EIO; // TODO
    });
}

/*****************************************************/
int fuse_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << " size:" << std::to_string(size) 
        << " offset:" << std::to_string(off) << ")"; debug.Info();

    return standardTry([&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        return -EIO; // TODO
    });
}

/*****************************************************/
int fuse_fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry([&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        return -EIO; // TODO
    });
}

/*****************************************************/
#ifdef __APPLE__
int fuse_truncate(const char* path, off_t size)
#else
int fuse_truncate(const char* path, off_t size, struct fuse_file_info* fi)
#endif
{
    path++; debug << __func__ << "(path:" << path << " size:" << std::to_string(size) << ")"; debug.Info();

    return standardTry([&]()->int
    {
        File& file(rootPtr->GetFileByPath(path));

        return -EIO; // TODO
    });
}
