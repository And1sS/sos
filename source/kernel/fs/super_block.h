#ifndef SOS_SUPER_BLOCK_H
#define SOS_SUPER_BLOCK_H

#include "../lib/types.h"
#include "../synchronization/mutex.h"
#include "vfs.h"

// set once after initialisation has finished, safe to read without atomics when
// superblock has been obtained via vfs_type
#define SUPER_INITIALIZED (1 << 0)

#define SUPER_INITIALIZATION_FAILED (1 << 1)

// set when super block is no longer u
#define SUPER_DYING (1 << 2)

typedef struct vfs_super_block {
    // Immutable data
    u64 id;
    struct vfs_type* type;
    device* device;
    struct vfs_dentry* root; // Root dentry remains valid if there is any active
                             // reference to sb obtained via vfs_type
    // End of immutable data

    u64 flags;       // Should be accessed atomically
    u64 mount_count; // Should be accessed atomically
    u64 refc;        // Should be accessed atomically

    mutex rename_mut;

    linked_list_node self_node; // node that will be used in vfs_type list

    lock lock; // guards all fields below
    con_var initialization_cvar;
} vfs_super_block;

vfs_super_block* vfs_super_get(struct vfs_type* type, device* dev);
void vfs_super_unmount(vfs_super_block* sb);
void vfs_super_destroy(vfs_super_block* sb);

vfs_super_block* vfs_super_acquire(vfs_super_block* sb);
void vfs_super_release(vfs_super_block* sb);

void vfs_super_unlock_new(vfs_super_block* sb);
void vfs_super_unlock_failed(vfs_super_block* sb);

bool vfs_super_is_dying(vfs_super_block* sb);
bool vfs_super_is_initialization_failed(vfs_super_block* sb);

#endif // SOS_SUPER_BLOCK_H
