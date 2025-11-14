#ifndef SOS_MOUNT_H
#define SOS_MOUNT_H

#include "dcache/dentry.h"

typedef struct vfs_mount {
    // Immutable data
    struct vfs_mount* parent_mount;
    vfs_dentry* mounted_at;
    vfs_dentry* mount_root;
    // End of immutable data

    // node used in parent`s children list
    linked_list_node mount_node;

    rw_mutex mut;

    lock lock;
    linked_list children;
    ref_count refc;
} vfs_mount;

void vfs_mount_root(vfs_dentry* root);
vfs_mount* vfs_mount_create(vfs_mount* parent_mount, vfs_dentry* mounted_at,
                            vfs_dentry* mount_root);
vfs_mount* vfs_mount_get_root();

vfs_mount* vfs_mount_acquire(vfs_mount* mount);
void vfs_mount_release(vfs_mount* mount);

#endif // SOS_MOUNT_H
