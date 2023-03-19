
#include <errno.h>
#if WIN32
#define EHOSTDOWN EIO
#endif // WIN32

#include <bitset>
#include <functional>

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
#include "andromeda/filesystem/filedata/CacheManager.hpp"
using Andromeda::Filesystem::Filedata::CacheManager;

static Debug sDebug("FuseOperations",nullptr);

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
        SDBG_INFO(": " << fname << "... " << e.what()); return -EISDIR;
    }
    catch (const Folder::NotFolderException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOTDIR;
    }
    catch (const Folder::NotFoundException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOENT;
    }
    catch (const Folder::DuplicateItemException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -EEXIST;
    }
    catch (const Folder::ModifyException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOTSUP;
    }
    catch (const File::WriteTypeException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOTSUP;
    }
    catch (const Item::ReadOnlyException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -EROFS;
    }

    // Backend exceptions
    catch (const BackendImpl::UnsupportedException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOTSUP;
    }
    catch (const BackendImpl::ReadOnlyFSException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -EROFS;
    }
    catch (const BackendImpl::DeniedException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -EACCES;
    }
    catch (const BackendImpl::NotFoundException& e)  
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOENT;
    }
    catch (const CacheManager::MemoryException& e)
    {
        SDBG_INFO(": " << fname << "... " << e.what()); return -ENOMEM;
    }

    // Error exceptions
    catch (const HTTPRunner::ConnectionException& e)
    {
        SDBG_ERROR("... " << fname << "... " << e.what()); return -EHOSTDOWN;
    }
    catch (const BaseException& e) 
    // BaseRunner::EndpointException (HTTP endpoint errors)
    // BackendImpl::Exception (others should not happen)
    {
        SDBG_ERROR("... " << fname << "... " << e.what()); return -EIO;
    }
}

/** Class to track and print FUSE concurrency */
class ThreadsPrint // TODO remove me later?
{
public:
    inline ThreadsPrint(std::string func, Andromeda::Debug& debug) : 
        mFunc(func), mDebug(debug)
    {
        mDebug.Info([&](std::ostream& str)
        {
            std::unique_lock<std::mutex> llock(sMutex);
            ++sThreads; sPeakThreads = std::max(sThreads, sPeakThreads);
            str << mFunc << "() ENTER! threads:" << sThreads << " peakThreads:" << sPeakThreads;
        });
    }

    inline ~ThreadsPrint()
    {
        mDebug.Info([&](std::ostream& str)
        { 
            std::unique_lock<std::mutex> llock(sMutex);
            --sThreads; sPeakThreads = std::max(sThreads, sPeakThreads);
            str << mFunc << "() EXIT! threads:" << sThreads << " peakThreads:" << sPeakThreads; 
        });
    }

private:
    std::string mFunc;
    Andromeda::Debug& mDebug;

    static size_t sThreads;
    static size_t sPeakThreads;
    static std::mutex sMutex;
};

size_t ThreadsPrint::sThreads { 0 };
size_t ThreadsPrint::sPeakThreads { 0 };
std::mutex ThreadsPrint::sMutex;

/*****************************************************/
#if LIBFUSE2
void* FuseOperations::init(struct fuse_conn_info* conn)
#else
void* FuseOperations::init(struct fuse_conn_info* conn, struct fuse_config* cfg)
#endif // LIBFUSE2
{
    SDBG_INFO("()");
    ThreadsPrint(__func__,sDebug);

#if !LIBFUSE2
    conn->time_gran = 1000; // PHP microseconds
    cfg->negative_timeout = 1;
#endif // !LIBFUSE2

    SDBG_INFO("... conn->caps: " << std::bitset<32>(conn->capable));
    SDBG_INFO("... conn->want: " << std::bitset<32>(conn->want));

#if !LIBFUSE2
    conn->want &= static_cast<decltype(conn->want)>(~FUSE_CAP_HANDLE_KILLPRIV); // don't support setuid and setgid flags
#endif // !LIBFUSE2

    FuseAdapter& adapter { GetFuseAdapter() };

    adapter.SignalInit();
    return static_cast<void*>(&adapter);
}

/*****************************************************/
int FuseOperations::statfs(const char *path, struct statvfs* buf)
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    buf->f_namemax = 255;

    // TODO temporary garbage so windows writing works...
    #if WIN32
        // TODO maybe use page size for bsize?
        buf->f_bsize = 4096;
        buf->f_frsize = 4096;
        buf->f_blocks = 1024*1024*1024;
        buf->f_bfree = 1024*1024*1024;
        buf->f_bavail = 1024*1024*1024;
    #endif // WIN32
        
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

// TODO if Windows calls utimens then the conversion of timespec->double->timespec will not match
// maybe the server will need to actually store timespec sec/nsec...? may be important for syncing

/*****************************************************/
constexpr Item::Date timespec_to_date(const timespec& t)
{ 
    return static_cast<Item::Date>(t.tv_sec) + static_cast<Item::Date>(t.tv_nsec)/1e9; 
}

/*****************************************************/
constexpr void date_to_timespec(const Item::Date time, timespec& spec)
{
    spec.tv_sec = static_cast<decltype(spec.tv_sec)>(time); // truncate to int
    spec.tv_nsec = static_cast<decltype(spec.tv_nsec)>((time-static_cast<Item::Date>(spec.tv_sec))*1e9);
}

/*****************************************************/
static void item_stat(const Item::ScopeLocked& item, struct stat* stbuf)
{
    switch (item->GetType())
    {
        case Item::Type::FILE: stbuf->st_mode = S_IFREG; break;
        case Item::Type::FOLDER: stbuf->st_mode = S_IFDIR; break;
    }

    stbuf->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;
    
    if (item->isReadOnly()) stbuf->st_mode &= 
        static_cast<decltype(stbuf->st_mode)>(  // -Wsign-conversion
            ~S_IWUSR & ~S_IWGRP & ~S_IWOTH);

    stbuf->st_size = (item->GetType() == Item::Type::FILE) ? 
        static_cast<decltype(stbuf->st_size)>(
            dynamic_cast<const File&>(*item).GetSize()) : 0;

    stbuf->st_blocks = stbuf->st_size ? (stbuf->st_size-1)/512+1 : 0; // 512B blocks
    //stbuf->st_blksize = 4096; // TODO what to use? page size?

    Item::Date created { item->GetCreated() };
    Item::Date modified { item->GetModified() };
    Item::Date accessed { item->GetAccessed() };

#if WIN32
    date_to_timespec(created, stbuf->st_birthtim);
#endif // WIN32

#ifdef APPLE
    stbuf->st_ctime = static_cast<decltype(stbuf->st_ctime)>(created);
    stbuf->st_mtime = static_cast<decltype(stbuf->st_mtime)>(modified);
    stbuf->st_atime = static_cast<decltype(stbuf->st_atime)>(accessed);

    if (!stbuf->st_mtime) stbuf->st_mtime = stbuf->st_ctime;
    if (!stbuf->st_atime) stbuf->st_atime = stbuf->st_atime;
#else // !APPLE
    date_to_timespec(created, stbuf->st_ctim);
    date_to_timespec(modified, stbuf->st_mtim);
    date_to_timespec(accessed, stbuf->st_atim);
    
    if (modified == 0) stbuf->st_mtim = stbuf->st_ctim;
    if (accessed == 0) stbuf->st_atim = stbuf->st_ctim;
#endif // APPLE
}

/*****************************************************/
int FuseOperations::access(const char* path, int mask)
{
    SDBG_INFO("(path:" << path << ", mask:" << mask << ")");
    ThreadsPrint(__func__,sDebug);

    #if defined(W_OK)
        static const std::string fname(__func__);
    #endif // W_OK
    return standardTry(__func__,[&]()->int
    {
        #if defined(W_OK)
            const Item::ScopeLocked item(GetFuseAdapter().GetRootFolder().GetItemByPath(path));

            if ((mask & W_OK) && item->isReadOnly()) 
            {
                sDebug.Info([&](std::ostream& str){ 
                    str << fname << "... read-only!"; });
                return -EACCES;
            }
        #endif // W_OK
            
        return FUSE_SUCCESS;
    });
}

// TODO use -o default_permissions and get rid of both of these?

/*****************************************************/
int FuseOperations::open(const char* path, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ", flags:" << fi->flags << ")");
    ThreadsPrint(__func__,sDebug);

    static const std::string fname(__func__);
    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        // TODO need to handle O_APPEND?

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR) && file->isReadOnly())
        {
            sDebug.Info([&](std::ostream& str){ 
                str << fname << "... read-only!"; });
            return -EACCES;
        }

        if (fi->flags & O_TRUNC)
        {
            sDebug.Info([&](std::ostream& str){ 
                str << fname << "... truncating!"; });
            file->Truncate(0);
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::opendir(const char* path, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ", flags:" << fi->flags << ")");
    ThreadsPrint(__func__,sDebug);

    static const std::string fname(__func__);
    return standardTry(__func__,[&]()->int
    {
        const Folder::ScopeLocked folder(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR) && folder->isReadOnly())
        {
            sDebug.Info([&](std::ostream& str){ 
                str << fname << "... read-only!"; });
            return -EACCES;
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::getattr(const char* path, struct stat* stbuf)
#else
int FuseOperations::getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi)
#endif // LIBFUSE2
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

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
#endif // LIBFUSE2
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    static const std::string fname(__func__);
    return standardTry(__func__,[&]()->int
    {
        const Folder::LockedItemMap items(GetFuseAdapter().GetRootFolder().GetFolderByPath(path)->GetItems());

        sDebug.Info([&](std::ostream& str){ 
            str << fname << "... #items:" << items.size(); });

        for (const decltype(items)::value_type& pair : items)
        {
            const Item::ScopeLocked& item { pair.second };

#if LIBFUSE2
            int retval { filler(buf, item->GetName().c_str(), NULL, 0) };
#else
            int retval; if (flags & FUSE_READDIR_PLUS)
            {
                struct stat stbuf; item_stat(item, &stbuf);

                retval = filler(buf, item->GetName().c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS);
            }
            else retval = filler(buf, item->GetName().c_str(), NULL, 0, static_cast<fuse_fill_dir_flags>(0));
#endif // LIBFUSE2
            if (retval != FUSE_SUCCESS) { sDebug.Error([&](std::ostream& str){ 
                str << fname << "... filler() failed"; }); return -EIO; }
        }

        for (const char* name : {".",".."})
        {
#if LIBFUSE2
            int retval { filler(buf, name, NULL, 0) };
#else
            int retval { filler(buf, name, NULL, 0, static_cast<fuse_fill_dir_flags>(0)) };
#endif // LIBFUSE2
            if (retval != FUSE_SUCCESS) { sDebug.Error([&](std::ostream& str){ 
                str << fname << "... filler() failed"; }); return -EIO; }
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::create(const char* fullpath, mode_t mode, struct fuse_file_info* fi)
{
    const Utilities::StringPair pair(Utilities::splitPath(fullpath));
    const std::string& path { pair.first }; 
    const std::string& name { pair.second };

    SDBG_INFO("(path:" << path << ", name:" << name << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        Folder::ScopeLocked parent(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        parent->CreateFile(name); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::mkdir(const char* fullpath, mode_t mode)
{
    const Utilities::StringPair pair(Utilities::splitPath(fullpath));
    const std::string& path { pair.first }; 
    const std::string& name { pair.second };
    
    SDBG_INFO("(path:" << path << ", name:" << name << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        Folder::ScopeLocked parent(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        parent->CreateFolder(name); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::unlink(const char* path)
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFuseAdapter().GetRootFolder().GetFileByPath(path) };

        // need an Item lock to pass to delete, cast from File
        Item::ScopeLocked item { Item::ScopeLocked::FromChild(std::move(file)) };

        item->Delete(item); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::rmdir(const char* path)
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        Folder::ScopeLocked folder { GetFuseAdapter().GetRootFolder().GetFolderByPath(path) };

        if (folder->CountItems()) return -ENOTEMPTY;

        // need an Item lock to pass to delete, cast from Folder
        Item::ScopeLocked item { Item::ScopeLocked::FromChild(std::move(folder)) };

        item->Delete(item); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::rename(const char* oldpath, const char* newpath)
#else
int FuseOperations::rename(const char* oldpath, const char* newpath, unsigned int flags)
#endif // LIBFUSE2
{
    Utilities::StringPair pair0(Utilities::splitPath(oldpath));
    const std::string& oldPath { pair0.first };  
    const std::string& oldName { pair0.second };

    Utilities::StringPair pair1(Utilities::splitPath(newpath));
    const std::string& newPath { pair1.first };  
    const std::string& newName { pair1.second };

    ThreadsPrint(__func__,sDebug);

    SDBG_INFO("... oldPath:" << oldPath << ", oldName:" << oldName);
    SDBG_INFO("... newPath:" << newPath << ", newName:" << newName);

    return standardTry(__func__,[&]()->int
    {
        Item::ScopeLocked item(GetFuseAdapter().GetRootFolder().GetItemByPath(oldpath));

        if (oldPath != newPath && oldName != newName)
        {
            //Folder::ScopeLocked parent(GetFuseAdapter().GetRootFolder().GetFolderByPath(newPath));

            SDBG_ERROR("NOT SUPPORTED YET!");
            return -EIO; // TODO implement me
        }
        else if (oldPath != newPath)
        {
            Folder::ScopeLocked newParent(GetFuseAdapter().GetRootFolder().GetFolderByPath(newPath));

            item->Move(*newParent, true);
        }
        else if (oldName != newName) 
        {
            item->Rename(newName, true);
        }

        return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ", offset:" << off << ", size:" << size << ")");
    ThreadsPrint(__func__,sDebug);

    if (off < 0) return -EINVAL;

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        return static_cast<int>(file->ReadBytesMax(buf, static_cast<uint64_t>(off), size));
    });
}

/*****************************************************/
int FuseOperations::write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ", offset:" << off << ", size:" << size << ")");
    ThreadsPrint(__func__,sDebug);

    if (off < 0) return -EINVAL;

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file->WriteBytes(buf, static_cast<uint64_t>(off), size); 
        
        return static_cast<int>(size);
    });
}

// TODO maybe should only FlushCache() on fsync, not flush? seems to be flush
// is only for applications->OS and has nothing to do with the storage "media"

/*****************************************************/
int FuseOperations::flush(const char* path, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file->FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::fsync(const char* path, int datasync, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file->FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::fsyncdir(const char* path, int datasync, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        Folder::ScopeLocked folder(GetFuseAdapter().GetRootFolder().GetFolderByPath(path));

        folder->FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
int FuseOperations::release(const char* path, struct fuse_file_info* fi)
{
    SDBG_INFO("(path:" << path << ", flags:" << fi->flags << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file->FlushCache(); return FUSE_SUCCESS;
    });
}

/*****************************************************/
void FuseOperations::destroy(void* private_data)
{
    SDBG_INFO("()");
    ThreadsPrint(__func__,sDebug);

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
#endif // LIBFUSE2
{
    SDBG_INFO("(path:" << path << ", size:" << size << ")");
    ThreadsPrint(__func__,sDebug);

    if (size < 0) return -EINVAL;

    return standardTry(__func__,[&]()->int
    {
        File::ScopeLocked file(GetFuseAdapter().GetRootFolder().GetFileByPath(path));

        file->Truncate(static_cast<uint64_t>(size)); return FUSE_SUCCESS;
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::chmod(const char* path, mode_t mode)
#else
int FuseOperations::chmod(const char* path, mode_t mode, struct fuse_file_info* fi)
#endif // LIBFUSE2
{
    FuseAdapter& fuseAdapter { GetFuseAdapter() };
    if (!fuseAdapter.GetOptions().fakeChmod) return -ENOTSUP;

    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        fuseAdapter.GetRootFolder().GetFileByPath(path); return FUSE_SUCCESS; // no-op
    });
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::chown(const char* path, uid_t uid, gid_t gid)
#else
int FuseOperations::chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi)
#endif // LIBFUSE2
{
    FuseAdapter& fuseAdapter { GetFuseAdapter() };
    if (!fuseAdapter.GetOptions().fakeChown) return -ENOTSUP;

    SDBG_INFO("(path:" << path << ")");
    ThreadsPrint(__func__,sDebug);

    return standardTry(__func__,[&]()->int
    {
        fuseAdapter.GetRootFolder().GetFileByPath(path); return FUSE_SUCCESS; // no-op
    });
}

} // namespace AndromedaFuse
