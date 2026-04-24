#ifndef SOS_MOUNT_H
#define SOS_MOUNT_H

#include "../synchronization/rw_spin_lock.h"
#include "dcache/dentry.h"

typedef struct vfs_mount {
    // Immutable data
    vfs_dentry* mount_root;
    // End of immutable data

    linked_list_node mount_node; // node used in parent`s children list, guarded
                                 // by parent`s mount lock and mount_mut

    u64 refc; // Should be accessed atomically

    lock lock; // guards all the fields below

    // these three should be modified only under both mount_mut in exclusive
    // mode and this mount lock
    struct vfs_mount* parent_mount;
    vfs_dentry* mounted_at;

    linked_list children;
} vfs_mount;

void vfs_mount_root(vfs_dentry* root);
vfs_mount* vfs_mount_get_root();

vfs_mount* vfs_mount_attach(vfs_mount* parent_mount, vfs_dentry* mounted_at,
                            vfs_dentry* mount_root);

vfs_mount* vfs_mount_acquire(vfs_mount* mount);
void vfs_mount_release(vfs_mount* mount);

u64 vfs_mount_find(vfs_mount* mount, vfs_dentry* dentry,
                      struct vfs_path* res);

#endif // SOS_MOUNT_H
