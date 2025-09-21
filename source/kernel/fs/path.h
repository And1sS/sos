#ifndef SOS_PATH_H
#define SOS_PATH_H

#include "mount.h"

#define MAX_PATH_LENGTH 512

typedef struct vfs_path {
    vfs_mount* mount;
    struct vfs_dentry* dentry;
} vfs_path;

// temporary structure for path walking
typedef struct path_parts {
    char part[MAX_PATH_LENGTH];
    u64 parts_left;
    string path;
} path_parts;

path_parts path_parts_from_path(string path);

u64 walk_one(vfs_path start, vfs_path* res, path_parts* parts);
u64 walk_parent(vfs_path start, vfs_path* res, path_parts* parts);
u64 walk(vfs_path start, vfs_path* res, path_parts* parts);

vfs_path vfs_path_create(vfs_mount* mnt, struct vfs_dentry* dentry);
void vfs_path_release(vfs_path* path);

#endif // SOS_PATH_H
