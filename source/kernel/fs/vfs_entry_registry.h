//#ifndef SOS_VFS_ENTRY_REGISTRY_H
//#define SOS_VFS_ENTRY_REGISTRY_H
//
//#include "../error/errno.h"
//#include "../lib/container/hash_table/hash_table.h"
//#include "../lib/string.h"
//#include "../memory/heap/kheap.h"
//#include "../synchronization/mutex.h"
//#include "vfs.h"
//
//// cache is: per vfs, per node, per every child(path and id - wise)
//// if vfs_entry refc is > 0 then it is inside cache, otherwise it may be not in
//// cache
//typedef struct {
//    hash_table cache; // maps vfs id to vfs cache
//    lock lock;
//} cache;
//
//typedef struct {
//    hash_table vfs_cache; // maps ventry id to vfs node cache
//    lock lock;
//} vfs_cache;
//
//typedef struct {
//    hash_table path_cache; // maps child path to child cache entry
//    hash_table id_cache;   // maps child id to child cache entry
//    lock lock;
//} vfs_node_cache;
//
//typedef struct {
//    vfs_entry* entry;
//    mutex mut; // mutex to prevent multiple threads from concurrent creation
//} child_cache_entry;
//
//static cache entries_cache;
//
//void vfs_registry_init() {
//    hash_table_init(&entries_cache.cache);
//    entries_cache.lock = SPIN_LOCK_STATIC_INITIALIZER;
//}
//
//vfs_cache* vfs_cache_create() {
//    vfs_cache* cache = kmalloc(sizeof(vfs_cache));
//    if (!cache)
//        return NULL;
//
//    memset(cache, 0, sizeof(vfs_cache));
//    hash_table_init(&cache->vfs_cache);
//    cache->lock = SPIN_LOCK_STATIC_INITIALIZER;
//
//    return cache;
//}
//
//vfs_node_cache* vfs_node_cache_create() {
//    vfs_node_cache* node_cache = kmalloc(sizeof(vfs_node_cache));
//    if (!node_cache)
//        return NULL;
//
//    memset(node_cache, 0, sizeof(vfs_node_cache));
//    hash_table_init(&node_cache->id_cache);
//    hash_table_init(&node_cache->path_cache);
//    node_cache->lock = SPIN_LOCK_STATIC_INITIALIZER;
//
//    return node_cache;
//}
//
//void vfs_get_entry(vfs_entry* entry) {
//    bool interrupts_enabled = spin_lock_irq_save(&entry->lock);
//    ref_acquire(&entry->refc);
//    spin_unlock_irq_restore(&entry->lock, interrupts_enabled);
//}
//
//void vfs_release_entry(vfs_entry* entry) {
//    bool interrupts_enabled = spin_lock_irq_save(&entry->lock);
//    ref_release(&entry->refc);
//    spin_unlock_irq_restore(&entry->lock, interrupts_enabled);
//}
//
//child_cache_entry* child_entry_create() {
//    child_cache_entry* entry = kmalloc(sizeof(child_cache_entry));
//    if (!entry)
//        return NULL;
//
//    entry->entry = NULL;
//    entry->mut = MUTEX_STATIC_INITIALIZER;
//
//    return entry;
//}
//
//u64 vfs_lookup_entry(vfs_entry* parent, string path, vfs_entry** result) {
//    spin_lock(&entries_cache.lock);
//    vfs_cache* vfs_cache = HASH_TABLE_GET_OR_PUT(
//        &entries_cache.cache, ((vfs*) parent->fs)->id, vfs_cache_create());
//    spin_unlock(&entries_cache.lock);
//
//    if (!vfs_cache)
//        return -ENOMEM;
//
//    spin_lock(&vfs_cache->lock);
//    vfs_node_cache* node_cache = HASH_TABLE_GET_OR_PUT(
//        &vfs_cache->vfs_cache, parent->inode->id, vfs_node_cache_create());
//    spin_unlock(&vfs_cache->lock);
//
//    if (!node_cache)
//        return -ENOMEM;
//
//    spin_lock(&node_cache->lock);
//    child_cache_entry* entry_cache = HASH_TABLE_GET_OR_PUT(
//        &node_cache->path_cache, strhash(path), child_entry_create());
//    spin_unlock(&node_cache->lock);
//
//    if (!entry_cache)
//        return -ENOMEM;
//
//    mutex_lock(&entry_cache->mut);
//    vfs_entry* existing = entry_cache->entry;
//    if (existing) {
//        vfs_get_entry(existing);
//        mutex_unlock(&entry_cache->mut);
//
//        *result = existing;
//        return 0;
//    }
//
//    if (!parent->inode->ops->lookup)
//        return -ENOSYS;
//
//    u64 lookup_result = parent->inode->ops->lookup(
//        (struct vfs_inode*) parent->inode, path, (struct vfs_entry**) result);
//    if (lookup_result != 0) {
//        mutex_unlock(&entry_cache->mut);
//        return lookup_result;
//    }
//
//    // TODO: also add to id cache
//    entry_cache->entry = *result;
//    vfs_get_entry(*result);
//    mutex_unlock(&entry_cache->mut);
//
//    return 0;
//}
//
//#endif // SOS_VFS_ENTRY_REGISTRY_H
