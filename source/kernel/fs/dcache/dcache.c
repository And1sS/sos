#include "dcache.h"
#include "../../lib/hash.h"
#include "../../lib/string.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
typedef struct {
    array_list buckets;
    u64 size;
} dcache;

dcache cache;

void vfs_dcache_init(u64 max_entries) {
    array_list_init(&cache.buckets, 1024);
    cache.size = 1024;
}

static u64 dcache_hash(vfs_dentry* parent, string name) {
    return hash2((u64) parent, strhash(name));
}

static u64 dcache_entry_hash(dcache_hash_entry* entry) {
    return dcache_hash(entry->parent, entry->name);
}

vfs_dentry* vfs_dcache_put(vfs_dentry* dentry) {
    dcache_hash_entry* entry = dentry->hash_entry;
    entry->parent = dentry->parent;
    entry->name = dentry->name;

    u64 bucket = dcache_entry_hash(entry) % cache.buckets.size;

    dcache_hash_entry* head = array_list_get(&cache.buckets, bucket);
    dcache_hash_entry* iter = head;

    entry->prev = NULL;
    entry->next = NULL;

    if (!head) {
        array_list_set(&cache.buckets, bucket, entry);
        return NULL;
    }

    while (iter) {
        if (entry->parent == iter->parent && streq(entry->name, iter->name)) {
            entry->prev = iter->prev;
            entry->next = iter->next;

            entry->prev->next = entry;
            if (entry->next)
                entry->next->prev = entry;

            iter->prev = NULL;
            iter->next = NULL;
            vfs_dentry* replaced = iter->child;
            replaced->hashed = false;

            return replaced;
        }

        iter = iter->next;
    }

    entry->next = head;
    head->prev = entry;
    array_list_set(&cache.buckets, 0, entry);
    return NULL;
}

// parent reference should be held during this routine
vfs_dentry* vfs_dcache_lookup(vfs_dentry* parent, string name) {
    u64 bucket = dcache_hash(parent, name) % cache.buckets.size;
    dcache_hash_entry* iter = array_list_get(&cache.buckets, bucket);

    while (iter) {
        if (parent == iter->parent && streq(name, iter->name))
            return iter->child;

        iter = iter->next;
    }

    return NULL;
}

void vfs_dcache_unhash(vfs_dentry* dentry) {
    if (!dentry->hashed)
        return;

    dentry->hashed = false;

    dcache_hash_entry* entry = dentry->hash_entry;
    u64 bucket = dcache_entry_hash(entry) % cache.buckets.size;

    dcache_hash_entry* head = array_list_get(&cache.buckets, bucket);
    if (head == entry) {
        array_list_set(&cache.buckets, bucket, NULL);
        return;
    }

    if (!entry->next && !entry->prev)
        return;

    if (entry->prev)
        entry->prev->next = entry->next;

    if (entry->next)
        entry->next->prev = entry->prev;

    entry->prev = NULL;
    entry->next = NULL;
}

void vfs_dcache_rehash(vfs_dentry* dentry) {
    vfs_dcache_unhash(dentry);
    vfs_dcache_put(dentry);
}

#pragma clang diagnostic pop