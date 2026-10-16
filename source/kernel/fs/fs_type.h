#ifndef SOS_FS_TYPE_H
#define SOS_FS_TYPE_H

#include "../device/device.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/string.h"
#include "../synchronization/spin_lock.h"

// set when fs type is dying and can't be used for mounting anymore, after all
// references to fs type are gone, it will be removed from registry
#define FS_TYPE_DYING (1 << 0)

struct vfs_type;
struct vfs_super_block;

typedef struct {
    struct vfs_dentry* (*mount)(struct vfs_type* type, device* dev);
    u64 (*unmount)(struct vfs_super_block* sb);
    u64 (*sync)(struct vfs_super_block* sb);
} vfs_type_ops;

typedef struct vfs_type {
    // Immutable data
    string name;

    vfs_type_ops* ops;
    // End of immutable data

    u64 refc;  // should be accessed atomically
    u64 flags; // should be accessed atomically

    lock lock; // guards all fields below
    linked_list super_blocks;
} vfs_type;

void vfs_type_registry_init();

bool vfs_type_register(vfs_type* type);
void vfs_type_deregister(vfs_type* type);

vfs_type* vfs_type_get(string name);
vfs_type* vfs_type_acquire(vfs_type* type);
void vfs_type_release(vfs_type* type);

#endif // SOS_FS_TYPE_H
