
#include <cerrno>
#if WIN32
#define EHOSTDOWN EIO
#endif // WIN32

#include <bitset>
#include <functional>

#include "FuseAdapter.hpp"
#include "FuseOperations.hpp"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/SharedMutex.hpp"
using Andromeda::SharedLockR;
using Andromeda::SharedLockW;
#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;
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

namespace AndromedaFuse {

namespace { // anonymous

Debug sDebug("FuseOperations",nullptr); // NOLINT(cert-err58-cpp)

/*****************************************************/
inline FuseAdapter& GetFuseAdapter()
{
    return *static_cast<FuseAdapter*>(fuse_get_context()->private_data);
}

/*****************************************************/
inline Item::ScopeLocked GetItemByPath(const std::string& path)
{
    return GetFuseAdapter().GetRootFolder()->GetItemByPath(path);
}

/*****************************************************/
inline File::ScopeLocked GetFileByPath(const std::string& path)
{
    return GetFuseAdapter().GetRootFolder()->GetFileByPath(path);
}

/*****************************************************/
inline Folder::ScopeLocked GetFolderByPath(const std::string& path)
{
    return GetFuseAdapter().GetRootFolder()->GetFolderByPath(path);
}

/*****************************************************/
int CatchAsErrno(const char* const fname, const std::function<int()>& func, const char* const path)
{
    try { return func(); }

    #define SDBG_INFO_EXC(e) SDBG_INFO(": " << fname << "... " << path << ": " << e.what());
    #define SDBG_ERROR_EXC(e) SDBG_ERROR(": " << fname << "... " << path << ": " << e.what());

    // Item exceptions
    catch (const Folder::NotFileException& e)
    {
        SDBG_INFO_EXC(e); return -EISDIR;
    }
    catch (const Folder::NotFolderException& e)
    {
        SDBG_INFO_EXC(e); return -ENOTDIR;
    }
    catch (const Folder::NotFoundException& e)
    {
        SDBG_INFO_EXC(e); return -ENOENT;
    }
    catch (const Folder::DuplicateItemException& e)
    {
        SDBG_INFO_EXC(e); return -EEXIST;
    }
    catch (const Folder::ModifyException& e)
    {
        SDBG_ERROR_EXC(e); return -ENOTSUP;
    }
    catch (const File::WriteTypeException& e)
    {
        SDBG_ERROR_EXC(e); return -ENOTSUP;
    }
    catch (const Item::ReadOnlyFSException& e)
    {
        SDBG_INFO_EXC(e); return -EROFS;
    }
    catch (const Item::NullParentException& e)
    {
        SDBG_ERROR_EXC(e); return -ENOTSUP;
    }
    catch (const CacheManager::MemoryException& e)
    {
        SDBG_ERROR_EXC(e); return -ENOMEM;
    }

    // Backend exceptions
    catch (const BackendImpl::UnsupportedException& e)
    {
        SDBG_ERROR_EXC(e); return -ENOTSUP;
    }
    catch (const BackendImpl::ReadOnlyFSException& e)
    {
        SDBG_INFO_EXC(e); return -EROFS;
    }
    catch (const BackendImpl::DeniedException& e)
    {
        SDBG_INFO_EXC(e); return -EACCES;
    }
    catch (const BackendImpl::NotFoundException& e)  
    {
        SDBG_INFO_EXC(e); return -ENOENT;
    }
    catch (const BackendImpl::WriteSizeException& e)
    {
        SDBG_ERROR_EXC(e); return -ENOTSUP;
    }
    catch (const HTTPRunner::ConnectionException& e)
    {
        SDBG_ERROR_EXC(e); return -EHOSTDOWN;
    }

    catch (const BaseException& e) // anything else
    {
        SDBG_ERROR_EXC(e); return -EIO;
    }
}

} // anonymous namespace

/*****************************************************/
#if LIBFUSE2
void* FuseOperations::init(struct fuse_conn_info* conn)
#else
void* FuseOperations::init(struct fuse_conn_info* conn, struct fuse_config* cfg)
#endif // LIBFUSE2
{
    SDBG_INFO("()");

#if !LIBFUSE2
    conn->time_gran = 1000; // PHP microseconds
    cfg->negative_timeout = 1;
#endif // !LIBFUSE2

    SDBG_INFO("... conn->caps: " << std::bitset<32>(conn->capable));
    SDBG_INFO("... conn->want: " << std::bitset<32>(conn->want));

#if !LIBFUSE2
    conn->want &= ~static_cast<decltype(conn->want)>(FUSE_CAP_HANDLE_KILLPRIV); // don't support setuid and setgid flags
#endif // !LIBFUSE2

    FuseAdapter& adapter { GetFuseAdapter() };

    adapter.SignalInit();
    return static_cast<void*>(&adapter);
}

/*****************************************************/
int FuseOperations::statfs(const char *path, struct statvfs* buf)
{
    SDBG_INFO("(path:" << path << ")");

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
namespace { void item_stat(const Item::ScopeLocked& item, const SharedLockR& itemLock, struct stat* stbuf)
{    
    const Item::Type itemType { item->GetType() };
    if (itemType == Item::Type::FILE)
    {
        stbuf->st_mode = S_IFREG | static_cast<decltype(stbuf->st_mode)>(
            GetFuseAdapter().GetOptions().fileMode); 

        const File& file { dynamic_cast<const File&>(*item) };
        stbuf->st_size = static_cast<decltype(stbuf->st_size)>(file.GetSize(itemLock));
        stbuf->st_blksize = static_cast<decltype(stbuf->st_blksize)>(file.GetPageSize());
    }
    else if (itemType == Item::Type::FOLDER)
    {
        stbuf->st_mode = S_IFDIR | static_cast<decltype(stbuf->st_mode)>(
            GetFuseAdapter().GetOptions().dirMode);

        stbuf->st_size = 0;
        stbuf->st_blksize = 4096; // meaningless?
    }

    const fuse_context* fuse_context { fuse_get_context() };
    stbuf->st_uid = fuse_context->uid;
    stbuf->st_gid = fuse_context->gid;

    stbuf->st_blocks = !stbuf->st_size ? 0 :
        (stbuf->st_size-1)/512+1; // # of 512B blocks

    const Item::Date created { item->GetCreated(itemLock) };
    const Item::Date modified { item->GetModified(itemLock) };
    const Item::Date accessed { item->GetAccessed(itemLock) };

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
} } // anonymous namespace

/*****************************************************/
int FuseOperations::open(const char* const path, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ", flags:" << fi->flags << ")");

    const char* const fname { __func__ };
    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockW fileLock { file->GetWriteLock() };

        // TODO need to handle O_APPEND?

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR) && file->isReadOnlyFS()) // NOLINT(hicpp-signed-bitwise)
        {
            sDebug.Info([&](std::ostream& str){ 
                str << fname << "... read-only FS!"; });
            return -EROFS;
        }

        if (fi->flags & O_TRUNC) // NOLINT(hicpp-signed-bitwise)
        {
            sDebug.Info([&](std::ostream& str){ 
                str << fname << "... truncating!"; });
            file->Truncate(0, fileLock);
        }

        return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
int FuseOperations::opendir(const char* const path, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ", flags:" << fi->flags << ")");

    const char* const fname { __func__ };
    return CatchAsErrno(__func__,[&]()->int
    {
        const Folder::ScopeLocked folder { GetFolderByPath(path) };

        if ((fi->flags & O_WRONLY || fi->flags & O_RDWR) && folder->isReadOnlyFS()) // NOLINT(hicpp-signed-bitwise)
        {
            sDebug.Info([&](std::ostream& str){ 
                str << fname << "... read-only FS!"; });
            return -EROFS;
        }

        return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::getattr(const char* const path, struct stat* stbuf)
#else
int FuseOperations::getattr(const char* const path, struct stat* stbuf, struct fuse_file_info* const fi)
#endif // LIBFUSE2
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        Item::ScopeLocked item { GetItemByPath(path) };
        item_stat(item, item->GetReadLock(), stbuf); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::readdir(const char* const path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* const fi)
#else
int FuseOperations::readdir(const char* const path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* const fi, enum fuse_readdir_flags flags)
#endif // LIBFUSE2
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    const char* const fname { __func__ };
    return CatchAsErrno(__func__,[&]()->int
    {
        Folder::LockedItemMap items; { // lock scope
            Folder::ScopeLocked parent { GetFolderByPath(path) };
            items = parent->GetItems(parent->GetWriteLock());
        }

        sDebug.Info([&](std::ostream& str){ 
            str << fname << "... #items:" << items.size(); });

        for (const Folder::LockedItemMap::value_type& pair : items)
        {
            const Item::ScopeLocked& item { pair.second };
            const SharedLockR itemLock { item->GetReadLock() };

#if LIBFUSE2
            int retval { filler(buf, item->GetName(itemLock).c_str(), NULL, 0) };
#else
            int retval = -1; 
            if (flags & FUSE_READDIR_PLUS)
            {
                struct stat stbuf{}; item_stat(item, itemLock, &stbuf);

                retval = filler(buf, item->GetName(itemLock).c_str(), &stbuf, 0, FUSE_FILL_DIR_PLUS);
            }
            else retval = filler(buf, item->GetName(itemLock).c_str(), nullptr, 0, 
                static_cast<fuse_fill_dir_flags>(0)); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
#endif // LIBFUSE2
            if (retval != FUSE_SUCCESS) { sDebug.Error([&](std::ostream& str){ 
                str << fname << "... filler() failed: " << retval; }); return -EIO; }
        }

        for (const char* name : {".",".."})
        {
#if LIBFUSE2
            int retval { filler(buf, name, NULL, 0) };
#else
            int retval { filler(buf, name, nullptr, 0, 
                static_cast<fuse_fill_dir_flags>(0)) }; // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
#endif // LIBFUSE2
            if (retval != FUSE_SUCCESS) { sDebug.Error([&](std::ostream& str){ 
                str << fname << "... filler() failed: " << retval; }); return -EIO; }
        }

        return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
int FuseOperations::create(const char* fullpath, mode_t mode, struct fuse_file_info* const fi)
{
    const StringUtil::StringPair pair(StringUtil::splitPath(fullpath));
    const std::string& path { pair.first }; 
    const std::string& name { pair.second };

    SDBG_INFO("(path:" << path << ", name:" << name << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        Folder::ScopeLocked parent { GetFolderByPath(path) };
        const SharedLockW parentLock { parent->GetWriteLock() };

        parent->CreateFile(name, parentLock); return FUSE_SUCCESS;
    }, fullpath);
}

/*****************************************************/
int FuseOperations::mkdir(const char* fullpath, mode_t mode)
{
    const StringUtil::StringPair pair(StringUtil::splitPath(fullpath));
    const std::string& path { pair.first }; 
    const std::string& name { pair.second };
    
    SDBG_INFO("(path:" << path << ", name:" << name << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        Folder::ScopeLocked parent { GetFolderByPath(path) };
        const SharedLockW parentLock { parent->GetWriteLock() };

        parent->CreateFolder(name, parentLock); return FUSE_SUCCESS;
    }, fullpath);
}

/*****************************************************/
int FuseOperations::unlink(const char* const path)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        SharedLockW fileLock { file->GetWriteLock() };

        // need an Item lock to pass to delete, cast from File
        Item::ScopeLocked item { Item::ScopeLocked::FromChild(std::move(file)) };

        item->Delete(item, fileLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
int FuseOperations::rmdir(const char* const path)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        Folder::ScopeLocked folder { GetFolderByPath(path) };
        SharedLockW folderLock { folder->GetWriteLock() };

        if (folder->CountItems(folderLock)) return -ENOTEMPTY;

        // need an Item lock to pass to delete, cast from Folder
        Item::ScopeLocked item { Item::ScopeLocked::FromChild(std::move(folder)) };

        item->Delete(item, folderLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::rename(const char* oldpath, const char* newpath)
#else
int FuseOperations::rename(const char* oldpath, const char* newpath, unsigned int flags)
#endif // LIBFUSE2
{
    const StringUtil::StringPair pair0(StringUtil::splitPath(oldpath));
    const std::string& oldParent { pair0.first };
    const std::string& oldName { pair0.second };

    const StringUtil::StringPair pair1(StringUtil::splitPath(newpath));
    const std::string& newParent { pair1.first };
    const std::string& newName { pair1.second };

    SDBG_INFO("... oldParent:" << oldParent << ", oldName:" << oldName);
    SDBG_INFO("... newParent:" << newParent << ", newName:" << newName);

    return CatchAsErrno(__func__,[&]()->int
    {
        if (oldParent != newParent && oldName != newName)
        {
            SDBG_ERROR("NOT SUPPORTED YET!"); // NOLINT(bugprone-lambda-function-name)
            return -EIO; // TODO implement me (must be a single step)
        }
        else if (oldParent != newParent)
        {
            // FUSE seems to check this for us, but just in case...
            if (StringUtil::startsWith(newParent, oldParent+"/"+oldName+"/")) // oldpath might not end with /
                return -EINVAL; // can't move directory into itself, could deadlock here too due to lock order

            // TODO could still deadlock here if "item" causes a refresh in a farther up parent that wants to delete newParent

            // lock ordering is important here! parent first
            Folder::ScopeLocked parent { GetFolderByPath(newParent) };
            Item::ScopeLocked item { GetItemByPath(oldpath) };
            SharedLockW itemLock { item->GetWriteLock() };
            item->Move(*parent, itemLock, true);
        }
        else if (oldName != newName) 
        {
            Item::ScopeLocked item { GetItemByPath(oldpath) };
            SharedLockW itemLock { item->GetWriteLock() };
            item->Rename(newName, itemLock, true);
        }

        return FUSE_SUCCESS;
    }, oldpath);
}

/*****************************************************/
int FuseOperations::read(const char* const path, char* buf, size_t size, off_t off, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ", offset:" << off << ", size:" << size << ")");

    if (off < 0) return -EINVAL;

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockR fileLock { file->GetReadLock() };

        return static_cast<int>(file->ReadBytesMax(buf, static_cast<uint64_t>(off), size, fileLock));
    }, path);
}

/*****************************************************/
int FuseOperations::write(const char* const path, const char* buf, size_t size, off_t off, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ", offset:" << off << ", size:" << size << ")");

    if (off < 0) return -EINVAL;

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockW fileLock { file->GetWriteLock() };

        file->WriteBytes(buf, static_cast<uint64_t>(off), size, fileLock); 
        
        return static_cast<int>(size);
    }, path);
}

// TODO maybe should only FlushCache() on fsync, not flush? seems to be flush
// is only for applications->OS and has nothing to do with the storage "media"

/*****************************************************/
int FuseOperations::flush(const char* const path, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockW fileLock { file->GetWriteLock() };

        file->FlushCache(fileLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
int FuseOperations::fsync(const char* const path, int datasync, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockW fileLock { file->GetWriteLock() };

        file->FlushCache(fileLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
int FuseOperations::fsyncdir(const char* const path, int datasync, struct fuse_file_info* const fi)
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        Folder::ScopeLocked folder { GetFolderByPath(path) };
        const SharedLockW folderLock { folder->GetWriteLock() };

        folder->FlushCache(folderLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
int FuseOperations::release(const char* const path, struct fuse_file_info* const fi) // cppcheck-suppress constParameterCallback
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ", flags:" << fi->flags << ", flush:" << fi->flush << ")");

    // TODO this does not seem right.  At least check fi->flush? maybe lowlevel only
    // if you keep it, add matching releasedir

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockW fileLock { file->GetWriteLock() };

        file->FlushCache(fileLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::truncate(const char* const path, off_t size)
#else
int FuseOperations::truncate(const char* const path, off_t size, struct fuse_file_info* const fi)
#endif // LIBFUSE2
{
    if (path == nullptr) return -EINVAL;
    SDBG_INFO("(path:" << path << ", size:" << size << ")");

    if (size < 0) return -EINVAL;

    return CatchAsErrno(__func__,[&]()->int
    {
        File::ScopeLocked file { GetFileByPath(path) };
        const SharedLockW fileLock { file->GetWriteLock() };

        file->Truncate(static_cast<uint64_t>(size), fileLock); return FUSE_SUCCESS;
    }, path);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::chmod(const char* const path, mode_t mode)
#else
int FuseOperations::chmod(const char* const path, mode_t mode, struct fuse_file_info* const fi)
#endif // LIBFUSE2
{
    if (path == nullptr) return -EINVAL;
    const FuseAdapter& fuseAdapter { GetFuseAdapter() };
    if (!fuseAdapter.GetOptions().fakeChmod) return -ENOTSUP;

    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        // do GetItem just to check it exists
        GetItemByPath(path); return FUSE_SUCCESS; // no-op
    }, path);
}

/*****************************************************/
#if LIBFUSE2
int FuseOperations::chown(const char* const path, uid_t uid, gid_t gid)
#else
int FuseOperations::chown(const char* const path, uid_t uid, gid_t gid, struct fuse_file_info* const fi)
#endif // LIBFUSE2
{
    if (path == nullptr) return -EINVAL;
    const FuseAdapter& fuseAdapter { GetFuseAdapter() };
    if (!fuseAdapter.GetOptions().fakeChown) return -ENOTSUP;

    SDBG_INFO("(path:" << path << ")");

    return CatchAsErrno(__func__,[&]()->int
    {
        // do GetItem just to check it exists
        GetItemByPath(path); return FUSE_SUCCESS; // no-op
    }, path);
}

} // namespace AndromedaFuse
