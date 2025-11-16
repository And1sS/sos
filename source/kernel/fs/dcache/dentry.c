#include "dentry.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/flagops.h"
#include "../../lib/hash.h"
#include "../../lib/string.h"

static u64 dentry_hash(vfs_dentry* parent, string name) {
    return hash2((u64) parent, strhash(name));
}

// takes parent lock and dentry lock returns parent dentry with unreleased refc
// for reasoning see description of `vfs_dentry_release_and_not_release_parent`
static vfs_dentry* vfs_dentry_destroy_and_not_release_parent(
    vfs_dentry* dentry) {

    // At this point dentry is unreachable for others through hash table or
    // unused list, but it is still reachable via parent->children list.
    // Safe to use parent without lock here since parent modification is done
    // under the lock, which release has been synchronized with lock acquiring
    // when last reference has been released. And even if this has been on other
    // cpu - then visibility is carried by scheduler.
    vfs_dentry* parent = dentry->parent;

    // if destroy is called on initialisation failure - parent may be null
    if (parent && parent != dentry) {
        spin_lock(&parent->lock);
        linked_list_remove_node(&parent->children, &dentry->dentry_node);
        spin_unlock(&parent->lock);
    }

    vfs_inode_release(dentry->inode);
    strfree(dentry->name);
    kfree(dentry);

    dcache_unreserve();

    // no need to release root dentry as it is already freed
    return parent != dentry ? parent : NULL;
}

// top level destroy function, should not be called inside other release/destroy
// functions or while holding dcache_lock, should be used when passed dentry is
// not in cache
void vfs_dentry_destroy(vfs_dentry* dentry) {
    vfs_dentry* parent = vfs_dentry_destroy_and_not_release_parent(dentry);
    vfs_dentry_release(parent);
}

static vfs_dentry* vfs_dentry_allocate(vfs_inode* inode, string name) {
    string name_copy = strcpy(name);
    if (!name_copy)
        return ERROR_PTR(-ENOMEM);

    vfs_dentry* dentry = kmalloc(sizeof(vfs_dentry));
    if (!dentry) {
        strfree(name_copy);
        return ERROR_PTR(-ENOMEM);
    }

    memset(dentry, 0, sizeof(vfs_dentry));

    dentry->name = name_copy;
    dentry->inode = inode;
    vfs_inode_acquire(inode);

    dentry->unused_node = LINKED_LIST_NODE_OF(dentry);
    memset(&dentry->hash_entry, 0, sizeof(dcache_hash_entry));
    dentry->hash_entry.name = name_copy;

    dentry->flags = 0;
    dentry->refc = 0;

    dentry->lock = SPIN_LOCK_STATIC_INITIALIZER;
    dentry->hash_bucket = NULL;

    dentry->dentry_node = LINKED_LIST_NODE_OF(dentry);
    dentry->children = LINKED_LIST_STATIC_INITIALIZER;

    return vfs_dentry_acquire(dentry);
}

vfs_dentry* vfs_dentry_create_root(vfs_inode* inode) {
    vfs_dentry* dentry = ERROR_PTR(-ENOMEM);
    if (!dcache_reserve())
        return dentry;

    dentry = vfs_dentry_allocate(inode, "/");
    if (IS_ERROR(dentry)) {
        dcache_unreserve();
        return dentry;
    }

    dentry->parent = dentry;
    dcache_bucket* bucket = dcache_bucket_get(dentry_hash(dentry, "/"));

    dentry->hash_bucket = bucket;
    dcache_bucket_lock(bucket);
    dcache_put(bucket, dentry);
    dcache_bucket_unlock(bucket);

    return dentry;
}

vfs_dentry* vfs_dentry_create(vfs_dentry* parent, vfs_inode* inode,
                              string name) {

    dcache_bucket* bucket = dcache_bucket_get(dentry_hash(parent, name));

    dcache_bucket_lock(bucket);
    vfs_dentry* dentry = dcache_lookup(bucket, parent, name);
    if (dentry) {
        vfs_dentry_acquire(dentry);
        goto out;
    }
    dcache_bucket_unlock(bucket);

    bool allocate = dcache_reserve();

    dcache_bucket_lock(bucket);
    // need to recheck if dentry has not been added, since registration breaks
    // atomicity
    if ((dentry = dcache_lookup(bucket, parent, name))) {
        if (allocate)
            dcache_unreserve();

        vfs_dentry_acquire(dentry);
        goto out;
    }

    dentry = vfs_dentry_allocate(inode, name);
    if (IS_ERROR(dentry)) {
        dcache_unreserve();
        goto out;
    }

    dentry->parent = vfs_dentry_acquire(parent);
    dentry->hash_bucket = bucket;
    dcache_put(bucket, dentry);

    spin_lock(&parent->lock);
    linked_list_add_last_node(&parent->children, &dentry->dentry_node);
    spin_unlock(&parent->lock);

out:
    dcache_bucket_unlock(bucket);

    return dentry;
}

void vfs_dentry_move(vfs_dentry* new_parent, vfs_dentry* child, string name) {
    // Safe to do plain reads of parent and name since caller already holds both
    // old and new parents inode->rw_mut which carry visibility and prevent
    // concurrent changes
    vfs_dentry* old_parent = child->parent;

    // remove old link and point child towards new parent
    spin_lock(&old_parent->lock);
    linked_list_remove_node(&old_parent->children, &child->dentry_node);
    spin_unlock(&old_parent->lock);

    dcache_bucket* old_bucket =
        dcache_bucket_get(dentry_hash(old_parent, child->name));
    dcache_bucket* new_bucket =
        dcache_bucket_get(dentry_hash(new_parent, name));

    dcache_buckets_lock(old_bucket, new_bucket);

    // remove existing child
    vfs_dentry* existing_child = dcache_lookup(new_bucket, new_parent, name);
    if (existing_child) {
        spin_lock(&new_parent->lock);
        linked_list_remove_node(&new_parent->children,
                                &existing_child->dentry_node);
        spin_unlock(&new_parent->lock);

        spin_lock(&existing_child->lock);
        dcache_remove(new_bucket, existing_child);
        existing_child->parent = existing_child;
        existing_child->hash_bucket = NULL;
        spin_unlock(&existing_child->lock);
    }

    // rename child and put in dcache
    spin_lock(&child->lock);
    dcache_remove(old_bucket, child);
    strfree(child->name);
    child->name = name;
    child->parent = new_parent;
    child->hash_bucket = new_bucket;
    dcache_put(new_bucket, child);
    spin_unlock(&child->lock);

    spin_lock(&new_parent->lock);
    linked_list_add_last_node(&new_parent->children, &child->dentry_node);
    spin_unlock(&new_parent->lock);

    dcache_buckets_unlock(old_bucket, new_bucket);

    if (!existing_child)
        vfs_dentry_acquire(new_parent);

    vfs_dentry_release(old_parent);
    // no need to release new_parent since if we replaced some dentry then
    // parent reference just been moved to new child
}

void vfs_dentry_unlink(vfs_dentry* dentry) {
    // safe to do plain reads here since caller is holding parent inode->rw_mut
    // which carries visibility and prevents concurrent changes
    vfs_dentry* parent = dentry->parent;
    dcache_bucket* bucket = dentry->hash_bucket;

    dcache_bucket_lock(bucket);
    spin_lock(&dentry->lock);

    dcache_remove(bucket, dentry);
    dentry->parent = dentry;
    dentry->hash_bucket = NULL;

    spin_unlock(&dentry->lock);
    dcache_bucket_unlock(bucket);

    vfs_dentry_release(parent);
}

vfs_dentry* vfs_dentry_parent(vfs_dentry* dentry) {
    spin_lock(&dentry->lock);
    // acquire done under lock since parent is guaranteed to be valid during
    // whole locked scope, which might not be the case after
    vfs_dentry* parent = vfs_dentry_acquire(dentry->parent);
    spin_unlock(&dentry->lock);

    return parent;
}

vfs_dentry* vfs_dentry_acquire(vfs_dentry* dentry) {
    atomic_increment(&dentry->refc);
    return dentry;
}

// This function is a tradeoff, releasing operation may cause chain reaction of:
// release -> delete -> release parent -> delete parent and so on, which:
//
// 1) can cause a deadlock, since at some point deletion requires dcache_lock to
// be held and so further deletions won't be able to acquire it - thus causing a
// deadlock.
//
// 2) can cause stack overflow in case of long chains of releasing-deleting.
//
// to mitigate this, this function is releasing current dentry, maybe causing
// reclamation of memory, and if so - returning unreleased parent dentry
// reference to be released by `vfs_dentry_release` in loop,
// this way there is no possibility for a deadlock and also no possibility for
// stack overflow
static vfs_dentry* vfs_dentry_release_and_not_release_parent(
    vfs_dentry* dentry) {

    // fast path, transition for any refc > 1
    if (atomic_decrement_not_one(&dentry->refc))
        return NULL;

    // slow path, transition 1 -> 0
    bool destroy = false;
    dcache_bucket* bucket;
retry_bucket_lock:
    bucket = READ_ONCE(dentry->hash_bucket);

    dcache_bucket_lock(bucket);
    spin_lock(&dentry->lock);
    if (dentry->hash_bucket != bucket) {
        spin_unlock(&dentry->lock);
        dcache_bucket_unlock(bucket);
        goto retry_bucket_lock;
    }

    if (atomic_decrement_and_get(&dentry->refc) != 0)
        goto out;

    // Initialization failure is always visible since all sbs are obtained via
    // vfs_super_get, dying is synchronized with dcache unused dentries eviction
    // - even if we miss flag update due to cache coherence delays (before
    // eviction), dentry will be picked up by shrinker. If eviction has already
    // started - then flag will be carried by bucket lock
    vfs_super_block* sb = dentry->inode->sb;
    if (bucket && dcache_has_space() && !vfs_super_is_dying(sb)
        && !vfs_super_is_initialization_failed(sb)) {
        dcache_put_unused(bucket, dentry);
        goto out;
    }

    SET_FLAGS(dentry->flags, DENTRY_DYING);
    destroy = true;

    if (!bucket)
        goto out;
    dcache_remove(bucket, dentry);
    dentry->hash_bucket = NULL;

out:
    spin_unlock(&dentry->lock);
    dcache_bucket_unlock(bucket);

    return destroy ? vfs_dentry_destroy_and_not_release_parent(dentry) : NULL;
}

// this function is based on vfs_dentry_release_and_not_release_parent and
// releasing dentry with possible chain of releases of all ancestors in a loop
void vfs_dentry_release(vfs_dentry* dentry) {
    vfs_dentry* parent = vfs_dentry_release_and_not_release_parent(dentry);

    while (parent) {
        parent = vfs_dentry_release_and_not_release_parent(parent);
    }
}

vfs_dentry* vfs_dentry_lookup(vfs_dentry* parent, string name) {
    dcache_bucket* bucket = dcache_bucket_get(dentry_hash(parent, name));

    dcache_bucket_lock(bucket);
    vfs_dentry* dentry = dcache_lookup(bucket, parent, name);
    if (dentry)
        vfs_dentry_acquire(dentry);
    dcache_bucket_unlock(bucket);

    return dentry;
}

bool vfs_dentry_is_root(vfs_dentry* dentry) {
    return dentry == dentry->inode->sb->root;
}

bool vfs_dentry_is_dir(vfs_dentry* dentry) {
    return dentry->inode->type == DIRECTORY;
}

bool vfs_dentry_is_ancestor(vfs_dentry* dentry, vfs_dentry* ancestor) {
    vfs_dentry* iter = vfs_dentry_acquire(dentry);
    bool result = false;
    vfs_dentry* parent;

    while (true) {
        parent = vfs_dentry_parent(iter);

        if (parent == ancestor) {
            result = true;
            break;
        } else if (parent == iter) { // detached node or root
            result = false;
            break;
        }

        vfs_dentry_release(iter);
        iter = parent;
    }

    vfs_dentry_release(parent);
    vfs_dentry_release(iter);

    return result;
}

bool vfs_dentry_is_orphaned(vfs_dentry* dentry) {
    // safe to do plain reads here since caller is holding inode->rw_mut or
    // parent inode->rw_mut which carries visibility and prevents concurrent
    // changes
    return dentry->parent == dentry && !vfs_dentry_is_root(dentry);
}

void vfs_dentry_set_mountpoint(vfs_dentry* dentry) {
    SET_FLAGS(dentry->flags, DENTRY_MOUNTPOINT);
}

bool vfs_dentry_is_mountpoint(vfs_dentry* dentry) {
    return TEST_FLAG(dentry->flags, DENTRY_MOUNTPOINT);
}
