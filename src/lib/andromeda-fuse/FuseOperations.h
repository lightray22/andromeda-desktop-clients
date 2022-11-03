#ifndef FUSEOPERATIONS_H_
#define FUSEOPERATIONS_H_

#include "libfuse_Includes.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#if USE_FUSE2
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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FUSEOPERATIONS_H