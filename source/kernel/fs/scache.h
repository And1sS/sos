#ifndef SOS_SCACHE_H
#define SOS_SCACHE_H

#include "../lib/types.h"
#include "vfs.h"

struct vfs_super_block {
    u64 id;
    vfs_type* type;

    device* device;

    struct vfs_dentry* root;

    lock lock;
    ref_count refc;
};

struct vfs_super_block* vfs_super_get(vfs_type* type);

void vfs_super_acquire(struct vfs_super_block* sb);
void vfs_super_release(struct vfs_super_block* sb);

#endif // SOS_SCACHE_H
