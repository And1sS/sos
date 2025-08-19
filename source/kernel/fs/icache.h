#ifndef SOS_ICACHE_H
#define SOS_ICACHE_H

#include "../lib/types.h"
#include "vfs.h"

typedef struct {
    u64 id;

    vfs_super_block* fs;
    vfs_inode_type type;
    vfs_inode_ops* ops;

    lock lock; // guards all fields below
    ref_count refc;
    ref_count aliasc;

    void* private_data;
} vfs_inode;

void vfs_inode_acquire(vfs_inode* inode);
void vfs_inode_release(vfs_inode* inode);

bool vfs_icache_init(u64 max_inodes);

vfs_inode* vfs_icache_get(vfs_super_block* sb, u64 id);
vfs_inode* vfs_icache_release(vfs_inode* inode);

#endif // SOS_ICACHE_H
