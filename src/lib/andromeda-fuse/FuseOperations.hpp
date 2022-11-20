#ifndef A2FUSE_FUSEOPERATIONS_H_
#define A2FUSE_FUSEOPERATIONS_H_

#include "libfuse_Includes.h"

namespace AndromedaFuse {

class FuseAdapter;

/** Static FUSE functions */
struct FuseOperations
{
    static int statfs(const char *path, struct statvfs* buf);
    static int access(const char* path, int mask);
    static int open(const char* path, struct fuse_file_info* fi);
    static int opendir(const char* path, struct fuse_file_info* fi);
    static int create(const char* path, mode_t mode, struct fuse_file_info* fi);
    static int mkdir(const char* path, mode_t mode);
    static int unlink(const char* path);
    static int rmdir(const char* path);
    static int read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi);
    static int write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi);
    static int flush(const char* path, struct fuse_file_info* fi);
    static int fsync(const char* path, int datasync, struct fuse_file_info* fi);
    static int fsyncdir(const char* path, int datasync, struct fuse_file_info* fi);
    static void destroy(void* private_data);

    #if LIBFUSE2
    static void* init(struct fuse_conn_info* conn);
    static int getattr(const char* path, struct stat* stbuf);
    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
    static int rename(const char* oldpath, const char* newpath);
    static int truncate(const char* path, off_t size);
    static int chmod(const char* path, mode_t mode);
    static int chown(const char* path, uid_t uid, gid_t gid);
    #else
    static void* init(struct fuse_conn_info* conn, struct fuse_config* cfg);
    static int getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
    static int rename(const char* oldpath, const char* newpath, unsigned int flags);
    static int truncate(const char* path, off_t size, struct fuse_file_info* fi);
    static int chmod(const char* path, mode_t mode, struct fuse_file_info* fi);
    static int chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi);
    #endif
};

} // namespace AndromedaFuse

/** fuse_operations mapped to Andromeda functions */
struct a2fuse_operations : public fuse_operations
{
    a2fuse_operations() : fuse_operations() // zero-init base
    {
        getattr = AndromedaFuse::FuseOperations::getattr;
        mkdir = AndromedaFuse::FuseOperations::mkdir;
        unlink = AndromedaFuse::FuseOperations::unlink;
        rmdir = AndromedaFuse::FuseOperations::rmdir;
        rename = AndromedaFuse::FuseOperations::rename;
        chmod = AndromedaFuse::FuseOperations::chmod;
        chown = AndromedaFuse::FuseOperations::chown;
        truncate = AndromedaFuse::FuseOperations::truncate;
        open = AndromedaFuse::FuseOperations::open;
        read = AndromedaFuse::FuseOperations::read;
        write = AndromedaFuse::FuseOperations::write;
        statfs = AndromedaFuse::FuseOperations::statfs;
        flush = AndromedaFuse::FuseOperations::flush;
        fsync = AndromedaFuse::FuseOperations::fsync;
        opendir = AndromedaFuse::FuseOperations::opendir;
        readdir = AndromedaFuse::FuseOperations::readdir;
        fsyncdir = AndromedaFuse::FuseOperations::fsyncdir;
        init = AndromedaFuse::FuseOperations::init;
        destroy = AndromedaFuse::FuseOperations::destroy;
        access = AndromedaFuse::FuseOperations::access;
        create = AndromedaFuse::FuseOperations::create;
    }
};

#endif // A2FUSE_FUSEOPERATIONS_H
