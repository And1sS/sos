#ifndef SOS_ICACHE_H
#define SOS_ICACHE_H

#include "../lib/types.h"
#include "scache.h"
#include "vfs.h"

typedef struct {
    u64 id;

    struct vfs_super_block* sb;
    vfs_inode_type type;
    vfs_inode_ops* ops;

    lock lock; // guards all fields below

    bool initialised;

    ref_count refc;
    ref_count aliasc;

    void* private_data;
} vfs_inode;

void vfs_inode_acquire(vfs_inode* inode);
void vfs_inode_release(vfs_inode* inode);

bool vfs_icache_init(u64 max_inodes);

vfs_inode* vfs_icache_get(struct vfs_super_block* sb, u64 id);
vfs_inode* vfs_icache_release(vfs_inode* inode);

#endif // SOS_ICACHE_H
