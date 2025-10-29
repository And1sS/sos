#include "../../lib/math.h"
#include "../../lib/string.h"
#include "dentry.h"

static dcache cache;

static u64 alive_dentries = 0;
static u64 max_dentries = 0;

void dcache_init(u64 buckets, u64 max_entries) {
    if (!array_list_init(&cache.buckets, buckets))
        panic("Can't init dcache");

    for (u64 i = 0; i < buckets; i++) {
        dcache_bucket* bucket = kmalloc(sizeof(dcache_bucket));
        if (!bucket)
            panic("Can't init dcache");

        memset(bucket, 0, sizeof(dcache_bucket));
        bucket->lock = SPIN_LOCK_STATIC_INITIALIZER;
        array_list_add_last(&cache.buckets, bucket);
    }

    max_dentries = max_entries;
}

bool dcache_reserve() {
    if (atomic_increment_and_get(&alive_dentries) < atomic_get(&max_dentries))
        return true;

    if (!dcache_shrink()) {
        dcache_unreserve();
        return false;
    }

    return true;
}

void dcache_unreserve() { atomic_decrement(&alive_dentries); }

bool dcache_has_space() {
    return atomic_get(&alive_dentries) < atomic_get(&max_dentries);
}

dcache_bucket* dcache_bucket_get(u64 hash) {
    return array_list_get(&cache.buckets, hash % cache.buckets.size);
}

void dcache_bucket_lock(dcache_bucket* bucket) {
    if (!bucket)
        return;

    spin_lock(&bucket->lock);
}

void dcache_bucket_unlock(dcache_bucket* bucket) {
    if (!bucket)
        return;

    spin_unlock(&bucket->lock);
}

void dcache_buckets_lock(dcache_bucket* first, dcache_bucket* second) {
    dcache_bucket_lock(MIN(first, second));
    dcache_bucket_lock(MAX(first, second));
}

void dcache_buckets_unlock(dcache_bucket* first, dcache_bucket* second) {
    dcache_bucket_unlock(first);
    dcache_bucket_unlock(second);
}

void dcache_remove(dcache_bucket* bucket, vfs_dentry* dentry) {
    if (!bucket)
        return;

    dcache_remove_unused(bucket, dentry);

    dcache_hash_entry* entry = &dentry->hash_entry;
    dcache_hash_entry* head = bucket->head;

    if (head == entry) {
        bucket->head = NULL;
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

void dcache_remove_unused(dcache_bucket* bucket, vfs_dentry* dentry) {
    linked_list_remove_node(&bucket->unused_list, &dentry->unused_node);
}

vfs_dentry* dcache_pop_unused(dcache_bucket* bucket) {
    linked_list_node* head = linked_list_first(&bucket->unused_list);
    if (!head)
        return NULL;

    vfs_dentry* dentry = head->value;
    dcache_remove(dentry->hash_bucket, dentry);
    return head->value;
}

void dcache_put_unused(dcache_bucket* bucket, vfs_dentry* dentry) {
    if (!bucket)
        return;

    linked_list_add_last_node(&bucket->unused_list, &dentry->unused_node);
}

vfs_dentry* dcache_put(dcache_bucket* bucket, vfs_dentry* dentry) {
    dcache_hash_entry* entry = &dentry->hash_entry;
    entry->parent = dentry->parent;
    entry->name = dentry->name;
    entry->child = dentry;

    dcache_hash_entry* head = bucket->head;
    dcache_hash_entry* iter = head;

    entry->prev = NULL;
    entry->next = NULL;

    if (!iter) {
        bucket->head = entry;
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

            return iter->child;
        }

        iter = iter->next;
    }

    entry->next = head;
    head->prev = entry;
    bucket->head = entry;
    return NULL;
}

// parent reference should be held during this routine
vfs_dentry* dcache_lookup(dcache_bucket* bucket, vfs_dentry* parent,
                          string name) {

    dcache_hash_entry* iter = bucket->head;

    while (iter) {
        if (parent == iter->parent && streq(name, iter->name)) {
            vfs_dentry* result = iter->child;
            dcache_remove_unused(bucket, result);
            return result;
        }

        iter = iter->next;
    }

    return NULL;
}

extern void vfs_dentry_destroy(vfs_dentry* dentry);

bool dcache_shrink() {
    for (u64 i = 0; i < cache.buckets.size; i++) {
        dcache_bucket* bucket = dcache_bucket_get(i);

        dcache_bucket_lock(bucket);
        while (bucket->unused_list.size != 0) {
            vfs_dentry* unused_dentry = dcache_pop_unused(bucket);
            dcache_bucket_unlock(bucket);

            vfs_dentry_destroy(unused_dentry);

            return true;
        }
        dcache_bucket_unlock(bucket);
    }

    return false;
}