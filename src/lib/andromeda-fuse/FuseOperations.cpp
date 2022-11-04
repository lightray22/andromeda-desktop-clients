
#include <functional>
#include <bitset>

#include "FuseAdapter.hpp"
#include "FuseOperations.h"

#include "HTTPRunner.hpp"
#include "Backend.hpp"
#include "fsitems/Item.hpp"
#include "fsitems/File.hpp"
#include "fsitems/Folder.hpp"

static Debug debug("FuseOperations");

/*****************************************************/
static FuseAdapter* GetFuseAdapter()
{
    return (FuseAdapter*)(fuse_get_context()->private_data);
}

/*****************************************************/
int standardTry(const std::string& fname, std::function<int()> func)
{
    try { return func(); }

    // Item exceptions
    catch (const Folder::NotFileException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EISDIR;
    }
    catch (const Folder::NotFolderException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTDIR;
    }
    catch (const Folder::NotFoundException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOENT;
    }
    catch (const Folder::DuplicateItemException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EEXIST;
    }
    catch (const Folder::ModifyException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const File::WriteTypeException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const Item::ReadOnlyException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EROFS;
    }

    // Backend exceptions
    catch (const Backend::UnsupportedException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOTSUP;
    }
    catch (const Backend::ReadOnlyException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EROFS;
    }
    catch (const Backend::DeniedException& e)
    {
        debug << fname << "..." << e.what(); debug.Info(); return -EACCES;
    }
    catch (const Backend::NotFoundException& e)  
    {
        debug << fname << "..." << e.what(); debug.Info(); return -ENOENT;
    }

    // Error exceptions
    catch (const HTTPRunner::ConnectionException& e)
    {
        debug << fname << "..." << e.what(); debug.Error(); return -EHOSTDOWN;
    }
    catch (const Utilities::Exception& e)
    {
        debug << fname << "..." << e.what(); debug.Error(); return -EIO;
    }
}

/*****************************************************/
#if USE_FUSE2
void* a2fuse_init(struct fuse_conn_info* conn)
#else
void* a2fuse_init(struct fuse_conn_info* conn, struct fuse_config* cfg)
#endif
{
    debug << __func__ << "()"; debug.Info();

#if !USE_FUSE2
    conn->time_gran = 1000; // PHP microseconds
    cfg->negative_timeout = 1;
#endif

    return (void*)GetFuseAdapter();
}

/*****************************************************/
int a2fuse_statfs(const char *path, struct statvfs* buf)
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

    if (item.isReadOnly()) stbuf->st_mode &= ~S_IWUSR & ~S_IWGRP & ~S_IWOTH;

    stbuf->st_size = static_cast<off_t>(item.GetSize());

    stbuf->st_ctime = static_cast<time_t>(item.GetCreated());

    stbuf->st_mtime = static_cast<time_t>(item.GetModified());
    if (!stbuf->st_mtime) stbuf->st_mtime = stbuf->st_ctime;

    stbuf->st_atime = static_cast<time_t>(item.GetAccessed());
    if (!stbuf->st_atime) stbuf->st_atime = stbuf->st_ctime;
}

/*****************************************************/
int a2fuse_access(const char* path, int mask)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(GetFuseAdapter()->GetRootFolder().GetItemByPath(path));

        if (mask & W_OK && item.isReadOnly()) return -EROFS;

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_open(const char* path, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(GetFuseAdapter()->GetRootFolder().GetItemByPath(path));

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR)
            && item.isReadOnly()) return -EROFS;

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_opendir(const char* path, struct fuse_file_info* fi)
{
    return a2fuse_open(path, fi);
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_getattr(const char* path, struct stat* stbuf)
#else
int a2fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
#endif
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        item_stat(GetFuseAdapter()->GetRootFolder().GetItemByPath(path), stbuf); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
#else
int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags)
#endif
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        const Folder::ItemMap& items(GetFuseAdapter()->GetRootFolder().GetFolderByPath(path).GetItems());

        debug << __func__ << "... #items:" << items.size(); debug.Info();

        for (const Folder::ItemMap::value_type& pair : items)
        {
            const std::unique_ptr<Item>& item = pair.second;

            debug << __func__ << "... subitem: " << item->GetName(); debug.Info();
            
#if USE_FUSE2
            int retval = filler(buf, item->GetName().c_str(), NULL, 0);
#else
            int retval; if (flags & FUSE_READDIR_PLUS)
            {
                struct stat stbuf; item_stat(*item, &stbuf);

                retval = filler(buf, item->GetName().c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS);
            }
            else retval = filler(buf, item->GetName().c_str(), NULL, 0, (fuse_fill_dir_flags)0);
#endif
            if (retval != FUSE_SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        for (const char* name : {".",".."})
        {
#if USE_FUSE2
            int retval = filler(buf, name, NULL, 0);
#else
            int retval = filler(buf, name, NULL, 0, (fuse_fill_dir_flags)0);
#endif
            if (retval != FUSE_SUCCESS) { debug << __func__ << "... filler() failed"; debug.Error(); return -EIO; }
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_create(const char* fullpath, mode_t mode, struct fuse_file_info* fi)
{
    fullpath++; const Utilities::StringPair pair(Utilities::split(fullpath,"/",true));
    const std::string& path = pair.first; const std::string& name = pair.second;

    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& parent(GetFuseAdapter()->GetRootFolder().GetFolderByPath(path));

        parent.CreateFile(name); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_mkdir(const char* fullpath, mode_t mode)
{
    fullpath++; const Utilities::StringPair pair(Utilities::split(fullpath,"/",true));
    const std::string& path = pair.first; const std::string& name = pair.second;
    
    debug << __func__ << "(path:" << path << " name:" << name << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& parent(GetFuseAdapter()->GetRootFolder().GetFolderByPath(path));

        parent.CreateFolder(name); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_unlink(const char* path)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter()->GetRootFolder().GetFileByPath(path).Delete(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_rmdir(const char* path)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter()->GetRootFolder().GetFolderByPath(path).Delete(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_rename(const char* oldpath, const char* newpath)
#else
int a2fuse_rename(const char* oldpath, const char* newpath, unsigned int flags)
#endif
{
    oldpath++; const Utilities::StringPair pair0(Utilities::split(oldpath,"/",true));
    const std::string& path0 = pair0.first; const std::string& name0 = pair0.second;

    newpath++; const Utilities::StringPair pair1(Utilities::split(newpath,"/",true));
    const std::string& path1 = pair1.first; const std::string& name1 = pair1.second;

    debug << __func__ << "(oldpath:" << oldpath << " newpath:" << newpath << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Item& item(GetFuseAdapter()->GetRootFolder().GetItemByPath(oldpath));

        if (path0 != path1 && name0 != name1)
        {
            //Folder& parent(GetFuseAdapter()->GetRootFolder().GetFolderByPath(path1));

            debug << "NOT SUPPORTED YET!"; debug.Error(); return -EIO; // TODO
        }
        else if (path0 != path1)
        {
            Folder& parent(GetFuseAdapter()->GetRootFolder().GetFolderByPath(path1));

            item.Move(parent, true);
        }
        else if (name0 != name1) 
        {
            item.Rename(name1, true);
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << " offset:" << off << " size:" << size << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter()->GetRootFolder().GetFileByPath(path));

        return file.ReadBytes(reinterpret_cast<std::byte*>(buf), off, size);
    });
}

/*****************************************************/
int a2fuse_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << " offset:" << off << " size:" << size << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter()->GetRootFolder().GetFileByPath(path));

        file.WriteBytes(reinterpret_cast<const std::byte*>(buf), off, size); return size;
    });
}

/*****************************************************/
int a2fuse_flush(const char* path, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter()->GetRootFolder().GetFileByPath(path));

        file.FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter()->GetRootFolder().GetFileByPath(path));

        file.FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int a2fuse_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi)
{
    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        Folder& folder(GetFuseAdapter()->GetRootFolder().GetFolderByPath(path));

        folder.FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
void a2fuse_destroy(void* private_data)
{
    debug << __func__ << "()"; debug.Info();

    standardTry(__func__,[&]()->int
    {
        GetFuseAdapter()->GetRootFolder().FlushCache(true); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_truncate(const char* path, off_t size)
#else
int a2fuse_truncate(const char* path, off_t size, struct fuse_file_info* fi)
#endif
{
    path++; debug << __func__ << "(path:" << path << " size:" << size << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        File& file(GetFuseAdapter()->GetRootFolder().GetFileByPath(path));

        file.Truncate(size); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_chmod(const char* path, mode_t mode)
#else
int a2fuse_chmod(const char* path, mode_t mode, struct fuse_file_info* fi)
#endif
{
    if (!GetFuseAdapter()->GetOptions().fakeChmod) return -ENOTSUP;

    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter()->GetRootFolder().GetFileByPath(path); return FUSE_SUCCESS; // no-op
    });
}

/*****************************************************/
#if USE_FUSE2
int a2fuse_chown(const char* path, uid_t uid, gid_t gid)
#else
int a2fuse_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi)
#endif
{
    if (!GetFuseAdapter()->GetOptions().fakeChown) return -ENOTSUP;

    path++; debug << __func__ << "(path:" << path << ")"; debug.Info();

    return standardTry(__func__,[&]()->int
    {
        GetFuseAdapter()->GetRootFolder().GetFileByPath(path); return FUSE_SUCCESS; // no-op
    });
}
