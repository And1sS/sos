#ifndef SOS_DCACHE_H
#define SOS_DCACHE_H

#include "../../lib/types.h"
#include "dentry.h"

struct vfs_dentry;

typedef struct dcache_hash_entry {
    struct vfs_dentry* parent;
    string name;
    struct vfs_dentry* child;

    struct dcache_hash_entry* prev;
    struct dcache_hash_entry* next;
} dcache_hash_entry;

void vfs_dcache_init(u64 max_entries);

struct vfs_dentry* vfs_dcache_put(struct vfs_dentry* entry);

void vfs_dcache_unhash(struct vfs_dentry* entry);
void vfs_dcache_rehash(struct vfs_dentry* entry);
struct vfs_dentry* vfs_dcache_lookup(struct vfs_dentry* parent, string name);

#endif // SOS_DCACHE_H
