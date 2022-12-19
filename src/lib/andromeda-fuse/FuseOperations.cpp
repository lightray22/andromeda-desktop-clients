
#include <errno.h>
#if WIN32
#define EHOSTDOWN EIO
#endif // WIN32

#include <functional>
#include <bitset>

#include "FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;
#include "FuseOperations.hpp"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/filesystem/Item.hpp"
using Andromeda::Filesystem::Item;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;

static Debug debug("FuseOperations",nullptr);

namespace AndromedaFuse {

/*****************************************************/
static FuseAdapter& GetFuseAdapter()
{
    return *static_cast<FuseAdapter*>(fuse_get_context()->private_data);
}

/*****************************************************/
static int standardTry(const std::string& fname, std::function<int()> func)
{
    try { return func(); }

    // Item exceptions
    catch (const Folder::NotFileException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -EISDIR;
    }
    catch (const Folder::NotFolderException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -ENOTDIR;
    }
    catch (const Folder::NotFoundException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -ENOENT;
    }
    catch (const Folder::DuplicateItemException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -EEXIST;
    }
    catch (const Folder::ModifyException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const File::WriteTypeException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const Item::ReadOnlyException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -EROFS;
    }

    // Backend exceptions
    catch (const BackendImpl::UnsupportedException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const BackendImpl::ReadOnlyException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -EROFS;
    }
    catch (const BackendImpl::DeniedException& e)
    {
        debug << fname << "... " << e.what(); debug.Info(); return -EACCES;
    }
    catch (const BackendImpl::NotFoundException& e)  
    {
        debug << fname << "... " << e.what(); debug.Info(); return -ENOENT;
    }

    // Error exceptions
    catch (const HTTPRunner::ConnectionException& e)
    {
        debug << fname << "... " << e.what(); debug.Error(); return -EHOSTDOWN;
    }
    catch (const BaseException& e)
    {
        debug << fname << "... " << e.what(); debug.Error(); return -EIO;
    }
}

/*****************************************************/
#if LIBFUSE2
void* FuseOperations::init(struct fuse_conn_info* conn)
#else
void* FuseOperations::init(struct fuse_conn_info* conn, struct fuse_config* cfg)
#endif // LIBFUSE2
{
    debug << __func__ << "()"; debug.Info();

#if !LIBFUSE2
    conn->time_gran = 1000; // PHP microseconds
    cfg->negative_timeout = 1;
#endif // !LIBFUSE2

    FuseAdapter& adapter { GetFuseAdapter() };

    adapter.SignalInit();
    return static_cast<void*>(&adapter);
}

/*****************************************************/
int FuseOperations::statfs(const char *path, struct statvfs* buf)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    buf->f_namemax = 255;
    
    // TODO temporary garbage so windows writing works...
    #if WIN32
        buf->f_bsize = 4096;
        buf->f_frsize = 4096;
        buf->f_blocks = 1024*1024*1024;
        buf->f_bfree = 1024*1024*1024;
        buf->f_bavail = 1024*1024*1024;
    #endif
        
    // The 'f_favail', 'f_fsid' and 'f_flag' fields are ignored   // TODO implement me
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

    return FUSE_SUCCESS;
}

#if WIN32
static void get_timespec(double time, fuse_timespec& spec)
{
    spec.tv_sec = static_cast<decltype(spec.tv_sec)>(time);
    spec.tv_nsec = static_cast<decltype(spec.tv_sec)>((time-spec.tv_sec)*1e9);
}
#endif // WIN32

/*****************************************************/
static void item_stat(const Item& item, struct stat* stbuf)
{
    switch (item.GetType())
    {
        case Item::Type::FILE: stbuf->st_mode = S_IFREG; break;
        case Item::Type::FOLDER: stbuf->st_mode = S_IFDIR; break;
    }

    stbuf->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;
    
    if (item.isReadOnly()) stbuf->st_mode &= 
        static_cast<decltype(stbuf->st_mode)>(~S_IWUSR & ~S_IWGRP & ~S_IWOTH); // -Wsign-conversion

    stbuf->st_size = static_cast<decltype(stbuf->st_size)>(item.GetSize());

    #if WIN32
        auto created { item.GetCreated() };
        auto modified { item.GetModified() };
        auto accessed { item.GetAccessed() };

        get_timespec(created, stbuf->st_birthtim);
        get_timespec(created, stbuf->st_ctim);
        get_timespec(modified, stbuf->st_mtim);
        get_timespec(accessed, stbuf->st_atim);
        
        if (!modified) stbuf->st_mtim = stbuf->st_ctim;
        if (!accessed) stbuf->st_atim = stbuf->st_ctim;
    #else
        stbuf->st_ctime = static_cast<decltype(stbuf->st_ctime)>(item.GetCreated());
        stbuf->st_mtime = static_cast<decltype(stbuf->st_mtime)>(item.GetModified());
        stbuf->st_atime = static_cast<decltype(stbuf->st_atime)>(item.GetAccessed());
        
        if (!stbuf->st_mtime) stbuf->st_mtime = stbuf->st_ctime;
        if (!stbuf->st_atime) stbuf->st_atime = stbuf->st_ctime;
    #endif
}

/*****************************************************/
int FuseOperations::access(const char* path, int mask)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ", mask: " << mask << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        #if defined(W_OK)
            Item& item(GetFuseAdapter().GetRootFolder().GetItemByPath(path));

            if ((mask & W_OK) && item.isReadOnly()) 
            {
                debug << __func__ << "... read-only!"; debug.Info(); return -EROFS;
            }
        #endif // W_OK
            
        return FUSE_SUCCESS;
    });
}

// TODO should close() be implemented and flush cache?

/*****************************************************/
int FuseOperations::open(const char* path, struct fuse_file_info* fi)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        const Item& item(GetFuseAdapter().GetRootFolder().GetItemByPath(path));

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR) && item.isReadOnly())
        {
            debug << __func__ << "... read-only!"; debug.Info(); return -EROFS;
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::opendir(const char* path, struct fuse_file_info* fi)
{
    return open(path, fi);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::getattr(const char* path, struct stat* stbuf)
#else
int FuseOperations::getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
#endif
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        item_stat(GetFuseAdapter().GetRootFolder().GetItemByPath(path), stbuf); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
#else
int FuseOperations::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
#endif
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        const Folder::ItemMap& items(GetFuseAdapter().GetRootFolder().GetFolderByPath(path).GetItems());

        debug << __func__ << "... #items:" << items.size(); debug.Info();

        for (const Folder::ItemMap::value_type& pair : items)
        {
            const std::unique_ptr<Item>& item { pair.second };

            debug << __func__ << "... subitem: " << item->GetName(); debug.Info();
            
#if LIBFUSE2
            int retval { filler(buf, item->GetName().c_str(), NULL, 0) };
#else
            int retval; if (flags & FUSE_READDIR_PLUS)
            {
                struct stat stbuf; item_stat(*item, &stbuf);

                retval = filler(buf, item->GetName().c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS);
            }
            else retval = filler(buf, item->GetName().c_str(), NULL, 0, static_cast<fuse_fill_dir_flags>(0));
#endif
            if (retval != FUSE_SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        for (const char* name : {".",".."})
        {
#if LIBFUSE2
            int retval { filler(buf, name, NULL, 0) };
#else
            int retval { filler(buf, name, NULL, 0, static_cast<fuse_fill_dir_flags>(0)) };
#endif
            if (retval != FUSE_SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::create(const char* fullpath, mode_t mode, struct fuse_file_info* fi)
{
    while (fullpath[0] == '/') fullpath++; 
    const Utilities::StringPair pair(Utilities::split(fullpath,"/",0,true));
    const std::string& path { pair.first }; 
    const std::string& name { pair.second };

    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& parent(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        parent.CreateFile(name); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::mkdir(const char* fullpath, mode_t mode)
{
    while (fullpath[0] == '/') fullpath++; 
    const Utilities::StringPair pair(Utilities::split(fullpath,"/",0,true));
    const std::string& path { pair.first }; 
    const std::string& name { pair.second };
    
    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& parent(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        parent.CreateFolder(name); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::unlink(const char* path)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter().GetRootFolder().GetFileByPath(path).Delete(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::rmdir(const char* path)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter().GetRootFolder().GetFolderByPath(path).Delete(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::rename(const char* oldpath, const char* newpath)
#else
int FuseOperations::rename(const char* oldpath, const char* newpath, unsigned int flags)
#endif
{
    while (oldpath[0] == '/') oldpath++; 
    const Utilities::StringPair pair0(Utilities::split(oldpath,"/",0,true));
    const std::string& oldPath { pair0.first }; 
    const std::string& oldName { pair0.second };

    while (newpath[0] == '/') newpath++; 
    const Utilities::StringPair pair1(Utilities::split(newpath,"/",0,true));
    const std::string& newPath { pair1.first };
    const std::string& newName { pair1.second };

    debug << __func__ << "(oldpath:" << oldpath << " newpath:" << newpath << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(GetFuseAdapter().GetRootFolder().GetItemByPath(oldpath));

        if (oldPath != newPath && oldName != newName)
        {
            //Folder& parent(GetFuseAdapter().GetRootFolder().GetFolderByPath(newPath));

            debug << "NOT SUPPORTED YET!"; debug.Error(); return -EIO; // TODO implement me
        }
        else if (oldPath != newPath)
        {
            Folder& newParent(GetFuseAdapter().GetRootFolder().GetFolderByPath(newPath));

            item.Move(newParent, true);
        }
        else if (oldName != newName) 
        {
            item.Rename(newName, true);
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << " offset:" << off << " size:" << size << ")"; debug.Info();

    if (off < 0) return -EINVAL;

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        return static_cast<int>(file.ReadBytes(reinterpret_cast<std::byte*>(buf), static_cast<uint64_t>(off), size));
    });
}

/*****************************************************/
int FuseOperations::write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << " offset:" << off << " size:" << size << ")"; debug.Info();

    if (off < 0) return -EINVAL;

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file.WriteBytes(reinterpret_cast<const std::byte*>(buf), static_cast<uint64_t>(off), size); 
        
        return static_cast<int>(size);
    });
}

/*****************************************************/
int FuseOperations::flush(const char* path, struct fuse_file_info* fi)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file.FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file.FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::fsyncdir(const char* path, int datasync, struct fuse_file_info* fi)
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& folder(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        folder.FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
void FuseOperations::destroy(void* private_data)
{
    debug << __func__ << "()"; debug.Info();

    standardTry(__func__,[&]()->int
    {
        FuseAdapter& adapter { *static_cast<FuseAdapter*>(private_data) };
        adapter.GetRootFolder().FlushCache(true); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::truncate(const char* path, off_t size)
#else
int FuseOperations::truncate(const char* path, off_t size, struct fuse_file_info* fi)
#endif
{
    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << " size:" << size << ")"; debug.Info();

    if (size < 0) return -EINVAL;

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file.Truncate(static_cast<uint64_t>(size)); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::chmod(const char* path, mode_t mode)
#else
int FuseOperations::chmod(const char* path, mode_t mode, struct fuse_file_info* fi)
#endif
{
    if (!GetFuseAdapter().GetOptions().fakeChmod) return -ENOTSUP;

    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter().GetRootFolder().GetFileByPath(path); return FUSE_SUCCESS; // no-op
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::chown(const char* path, uid_t uid, gid_t gid)
#else
int FuseOperations::chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi)
#endif
{
    if (!GetFuseAdapter().GetOptions().fakeChown) return -ENOTSUP;

    while (path[0] == '/') path++;
    debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter().GetRootFolder().GetFileByPath(path); return FUSE_SUCCESS; // no-op
    });
}

} // namespace AndromedaFuse
