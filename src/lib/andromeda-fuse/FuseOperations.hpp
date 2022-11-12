#ifndef FUSEOPERATIONS_H_
#define FUSEOPERATIONS_H_

#include "libfuse_Includes.h"

int a2fuse_statfs(const char *path, struct statvfs* buf);
int a2fuse_access(const char* path, int mask);
int a2fuse_open(const char* path, struct fuse_file_info* fi);
int a2fuse_opendir(const char* path, struct fuse_file_info* fi);
int a2fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi);
int a2fuse_mkdir(const char* path, mode_t mode);
int a2fuse_unlink(const char* path);
int a2fuse_rmdir(const char* path);
int a2fuse_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi);
int a2fuse_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi);
int a2fuse_flush(const char* path, struct fuse_file_info* fi);
int a2fuse_fsync(const char* path, int datasync, struct fuse_file_info* fi);
int a2fuse_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi);
void a2fuse_destroy(void* private_data);

#if LIBFUSE2
void* a2fuse_init(struct fuse_conn_info* conn);
int a2fuse_getattr(const char* path, struct stat* stbuf);
int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
int a2fuse_rename(const char* oldpath, const char* newpath);
int a2fuse_truncate(const char* path, off_t size);
int a2fuse_chmod(const char* path, mode_t mode);
int a2fuse_chown(const char* path, uid_t uid, gid_t gid);
#else
void* a2fuse_init(struct fuse_conn_info* conn, struct fuse_config* cfg);
int a2fuse_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi);
int a2fuse_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
int a2fuse_rename(const char* oldpath, const char* newpath, unsigned int flags);
int a2fuse_truncate(const char* path, off_t size, struct fuse_file_info* fi);
int a2fuse_chmod(const char* path, mode_t mode, struct fuse_file_info* fi);
int a2fuse_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi);
#endif

/** fuse_operations mapped to Andromeda functions */
struct a2fuse_operations : public fuse_operations
{
    a2fuse_operations() : fuse_operations() // zero-init base
    {
        getattr = a2fuse_getattr;
        mkdir = a2fuse_mkdir;
        unlink = a2fuse_unlink;
        rmdir = a2fuse_rmdir;
        rename = a2fuse_rename;
        chmod = a2fuse_chmod;
        chown = a2fuse_chown;
        truncate = a2fuse_truncate;
        open = a2fuse_open;
        read = a2fuse_read;
        write = a2fuse_write;
        statfs = a2fuse_statfs;
        flush = a2fuse_flush;
        fsync = a2fuse_fsync;
        opendir = a2fuse_opendir;
        readdir = a2fuse_readdir;
        fsyncdir = a2fuse_fsyncdir;
        init = a2fuse_init;
        destroy = a2fuse_destroy;
        access = a2fuse_access;
        create = a2fuse_create;
    }
};

#endif // FUSEOPERATIONS_H
