#ifndef SOS_PATH_H
#define SOS_PATH_H

#include "mount.h"

typedef struct {
    vfs_mount* mount;
    struct vfs_dentry* dentry;
} vfs_path;

u64 walk(vfs_path start, string path, vfs_path* res);

#endif // SOS_PATH_H
