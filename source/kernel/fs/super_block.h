#ifndef SOS_SUPER_BLOCK_H
#define SOS_SUPER_BLOCK_H

#include "../lib/types.h"
#include "vfs.h"

struct vfs_super_block {
    // Immutable data
    u64 id;
    struct vfs_type* type;
    device* device;
    struct vfs_dentry* root;
    // End of immutable data

    linked_list_node self_node; // node that will be used in vfs_type list

    lock lock; // guards all fields below
    bool dying;
    ref_count refc;
};

struct vfs_super_block* vfs_super_get(struct vfs_type* type, device* dev);
u64 vfs_super_destroy(struct vfs_super_block* sb);

void vfs_super_acquire(struct vfs_super_block* sb);
void vfs_super_release(struct vfs_super_block* sb);

#endif // SOS_SUPER_BLOCK_H
