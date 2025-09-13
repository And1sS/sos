#ifndef SOS_INODE_H
#define SOS_INODE_H

#include "../lib/types.h"
#include "super_block.h"
#include "vfs.h"

typedef struct {
    // Immutable data
    u64 id;

    struct vfs_super_block* sb;
    vfs_inode_type type;
    vfs_inode_ops* ops;
    // End of immutable data

    lock lock; // guards all fields below

    bool initialised;

    ref_count refc;
    ref_count aliasc;

    void* private_data;
} vfs_inode;

void vfs_inode_acquire(vfs_inode* inode);
void vfs_inode_release(vfs_inode* inode);

void vfs_icache_init(u64 max_inodes);

// Safe to use inside fill_super, since fill_super is called on unpublished
// superblock without superblock lock held
vfs_inode* vfs_icache_get(struct vfs_super_block* sb, u64 id);

#endif // SOS_INODE_H
