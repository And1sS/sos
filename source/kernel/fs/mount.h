#ifndef SOS_MOUNT_H
#define SOS_MOUNT_H

#include "dentry.h"

typedef struct vfs_mount {
    struct vfs_mount* parent_mount;
    struct vfs_dentry* parent;
    struct vfs_dentry* mount_root;

    lock lock;
    ref_count refc;
} vfs_mount;

void vfs_mount_root(struct vfs_dentry* root);
vfs_mount* vfs_mount_create(vfs_mount* parent_mount, struct vfs_dentry* parent,
                            struct vfs_dentry mount_root);
vfs_mount* vfs_mount_get_root();

void vfs_mount_acquire(vfs_mount* mount);
void vfs_mount_release(vfs_mount* mount);

#endif // SOS_MOUNT_H
