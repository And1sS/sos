#ifndef SOS_VFS_ENTRY_REGISTRY_H
#define SOS_VFS_ENTRY_REGISTRY_H

#include "../lib/container/hash_table/hash_table.h"
#include "../lib/string.h"
#include "../memory/heap/kheap.h"
#include "../synchronization/mutex.h"
#include "vfs.h"

// cache is: per vfs, per node, per every child(path and id - wise)
typedef struct {
    hash_table cache; // maps vfs id to vfs cache
    lock lock;
} cache;

typedef struct {
    hash_table vfs_cache; // maps ventry id to vfs node cache
    lock lock;
} vfs_cache;

typedef struct {
    hash_table path_cache; // maps child path to child cache entry
    hash_table id_cache;   // maps child id to child cache entry
    lock lock;
} vfs_node_cache;

typedef struct {
    vfs_entry* entry;
    mutex mut; // mutex to prevent multiple threads from concurrent creation
} child_cache_entry;

static cache entries_cache;

void vfs_registry_init() {
    hash_table_init(&entries_cache.cache);
    entries_cache.lock = SPIN_LOCK_STATIC_INITIALIZER;
}

vfs_cache* vfs_cache_create() {
    vfs_cache* cache = kmalloc(sizeof(vfs_cache));
    if (!cache)
        return NULL;

    memset(cache, 0, sizeof(vfs_cache));
    hash_table_init(&cache->vfs_cache);
    cache->lock = SPIN_LOCK_STATIC_INITIALIZER;

    return cache;
}

vfs_node_cache* vfs_node_cache_create() {
    vfs_node_cache* node_cache = kmalloc(sizeof(vfs_node_cache));
    if (!node_cache)
        return NULL;

    memset(node_cache, 0, sizeof(vfs_node_cache));
    hash_table_init(&node_cache->id_cache);
    hash_table_init(&node_cache->path_cache);
    node_cache->lock = SPIN_LOCK_STATIC_INITIALIZER;

    return node_cache;
}

child_cache_entry* child_entry_create() {
    child_cache_entry* entry = kmalloc(sizeof(child_cache_entry));
    if (!entry)
        return NULL;

    entry->entry = NULL;
    entry->mut = MUTEX_STATIC_INITIALIZER;

    return entry;
}

vfs_entry* vfs_cache_get_entry(vfs_entry* parent, string path,
                               lookup_func* lookup) {

    spin_lock(&entries_cache.lock);
    // TODO: handle out of memory
    vfs_cache* vfs_cache = HASH_TABLE_GET_OR_PUT(
        &entries_cache.cache, ((vfs*) parent->fs)->id, vfs_cache_create());
    spin_unlock(&entries_cache.lock);

    if (!vfs_cache)
        return NULL;

    spin_lock(&vfs_cache->lock);
    vfs_node_cache* node_cache = HASH_TABLE_GET_OR_PUT(
        &vfs_cache->vfs_cache, parent->inode->id, vfs_node_cache_create());
    spin_unlock(&vfs_cache->lock);

    if (!node_cache)
        return NULL;

    spin_lock(&node_cache->lock);
    child_cache_entry* entry_cache = HASH_TABLE_GET_OR_PUT(
        &node_cache->path_cache, strhash(path), child_entry_create());
    spin_unlock(&node_cache->lock);

    if (!entry_cache)
        return NULL;

    mutex_lock(&entry_cache->mut);
    if (entry_cache->entry) {
        mutex_unlock(&entry_cache->mut);
        return entry_cache->entry;
    }

    vfs_entry* created = kmalloc(sizeof(vfs_entry));
    // TODO: do all checks and cleanups in case of failure
    u64 result = lookup((struct vfs_inode*) parent->inode, path,
                        (struct vfs_entry**) &created);
    if (result != 0) {
        // TODO: error
    }

    // TODO: also add to id cache
    entry_cache->entry = created;
    spin_lock(&created->lock);
    ref_acquire(&created->refc);
    spin_unlock(&created->lock);

    mutex_unlock(&entry_cache->mut);

    return created;
}

#endif // SOS_VFS_ENTRY_REGISTRY_H
