
#include "FuseOperations.h"

const struct fuse_operations a2fuse_ops = {
    .getattr = a2fuse_getattr,
    .mkdir = a2fuse_mkdir,
    .unlink = a2fuse_unlink,
    .rmdir = a2fuse_rmdir,
    .rename = a2fuse_rename,
    .chmod = a2fuse_chmod,
    .chown = a2fuse_chown,
    .truncate = a2fuse_truncate,
    .open = a2fuse_open,
    .read = a2fuse_read,
    .write = a2fuse_write,
    .statfs = a2fuse_statfs,
    .flush = a2fuse_flush,
    .fsync = a2fuse_fsync,
    .opendir = a2fuse_opendir,
    .readdir = a2fuse_readdir,
    .fsyncdir = a2fuse_fsyncdir,
    .init = a2fuse_init,
    .destroy = a2fuse_destroy,
    .access = a2fuse_access,
    .create = a2fuse_create
};
