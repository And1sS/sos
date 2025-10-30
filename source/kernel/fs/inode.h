#ifndef SOS_INODE_H
#define SOS_INODE_H

#include "../lib/types.h"
#include "../synchronization/rw_mutex.h"
#include "super_block.h"
#include "vfs.h"

// set once after initialisation has finished, safe to read without atomics when
// inode has been obtained via cache
#define INODE_INITIALISED (1 << 0)

// set when inode is dead (from rmdir or unlink) and should not be used
// afterward
#define INODE_DEAD (1 << 1)

typedef struct vfs_inode {
    // Immutable data
    u64 id;

    struct vfs_super_block* sb;
    vfs_inode_type type;
    vfs_inode_ops* ops;
    // End of immutable data

    rw_mutex mut; // This mutex used for separating reading operations such as
                  // (lookup, readdir) from modifying operations (link, unlink,
                  // rmdir, etc.)

    u64 flags; // Should be accessed atomically
    u64 links; // Should be modified atomically and inspected only under mut
               // (exception is inspection before freeing, since noone can
               // modify links at that point and visibility will be carried
               // through refcount

    lock lock; // guards all fields below

    u64 mode;

    ref_count refc;

    void* private_data;

    con_var initialization_cvar;
} vfs_inode;

void vfs_inode_await_initialization(vfs_inode* inode);
// unlocks uninitialised inode, serializes all updated inode data, acts as
// store-release
vfs_inode* vfs_inode_unlock_new(vfs_inode* inode);

vfs_inode* vfs_inode_acquire(vfs_inode* inode);
void vfs_inode_release(vfs_inode* inode);

// agressively destroy inode caller got from vfs_icache_get,
// should be used when creation of new superblock is failed and
// created inode should be removed from cache and destroyed.
void vfs_inode_drop(vfs_inode* inode);

void vfs_inode_drop_link(vfs_inode* inode);
void vfs_inode_add_link(vfs_inode* inode);

void vfs_inode_lock_shared(vfs_inode* inode);
void vfs_inode_unlock_shared(vfs_inode* inode);
void vfs_inode_lock(vfs_inode* inode);
void vfs_inode_unlock(vfs_inode* inode);
void vfs_inodes_lock(vfs_inode* left, vfs_inode* right);
void vfs_inodes_unlock(vfs_inode* left, vfs_inode* right);

void vfs_icache_init(u64 max_inodes);

// Safe to use inside fill_super, since fill_super is called on unpublished
// superblock without superblock lock held
vfs_inode* vfs_icache_get(struct vfs_super_block* sb, u64 id);

#endif // SOS_INODE_H
