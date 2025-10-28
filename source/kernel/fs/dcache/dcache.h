#ifndef SOS_DCACHE_H
#define SOS_DCACHE_H

#include "../../lib/types.h"

struct vfs_dentry;

typedef struct dcache_hash_entry {
    struct vfs_dentry* parent;
    string name;
    struct vfs_dentry* child;

    struct dcache_hash_entry* prev;
    struct dcache_hash_entry* next;
} dcache_hash_entry;

typedef struct {
    lock lock;
    dcache_hash_entry* head;
    linked_list unused_list;
} dcache_bucket;

typedef struct {
    array_list buckets;
} dcache;

void dcache_init(u64 buckets, u64 max_entries);

bool dcache_reserve();
void dcache_unreserve();

dcache_bucket* dcache_bucket_get(u64 hash);

void dcache_bucket_lock(dcache_bucket* bucket);
void dcache_bucket_unlock(dcache_bucket* bucket);

void dcache_buckets_lock(dcache_bucket* left, dcache_bucket* right);
void dcache_buckets_unlock(dcache_bucket* left, dcache_bucket* right);

// All procedures below should be called with bucket locked
void dcache_remove(dcache_bucket* bucket, struct vfs_dentry* dentry);
void dcache_remove_unused(dcache_bucket* bucket, struct vfs_dentry* dentry);

struct vfs_dentry* dcache_put(dcache_bucket* bucket, struct vfs_dentry* dentry);
void dcache_put_unused(dcache_bucket* bucket, struct vfs_dentry* dentry);
struct vfs_dentry* dcache_pop_unused(dcache_bucket* bucket);

struct vfs_dentry* dcache_lookup(dcache_bucket* bucket,
                                 struct vfs_dentry* parent, string name);

bool dcache_evict();
bool dcache_has_space();

#endif // SOS_DCACHE_H
