#ifndef SOS_PATH_H
#define SOS_PATH_H

#include "mount.h"

typedef struct {
    vfs_mount* mount;
    struct vfs_dentry* dentry;
} vfs_path;

u64 walk(vfs_path start, string path, vfs_path* res);

vfs_path vfs_path_create(vfs_mount* mnt, struct vfs_dentry* dentry);
void vfs_path_release(vfs_path* path);

#endif // SOS_PATH_H
