#include "dentry.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
#include "../lib/string.h"

typedef struct {
    vfs_dentry* parent;
    string name;
} dcache_key;

static u64 dcache_hash(dcache_key key) {
    return hash2((u64) key.parent, strhash(key.name));
}

static bool dcache_equals(dcache_key a, dcache_key b) {
    return a.parent == b.parent && streq(a.name, b.name);
}

DEFINE_HASH_TABLE(dentry_cache, dcache_key, vfs_dentry*, dcache_hash,
                  dcache_equals)

static lock dcache_lock = SPIN_LOCK_STATIC_INITIALIZER;

static dentry_cache dcache;

static linked_list unused_dentries;
static u64 alive_dentries;
static u64 max_dentries;

static bool vfs_dcache_register_allocation_unsafe();
static void vfs_dcache_deregister_allocation_unsafe();

// takes parent lock and dentry lock returns parent dentry with unreleased refc
// for reasoning see description of `vfs_dentry_release_and_not_release_parent`
static vfs_dentry* vfs_dentry_destroy_and_not_release_parent(
    vfs_dentry* dentry) {

    // at this point dentry is unreachable for others through hash table or
    // unused list, but it is still reachable via parent->children list, so each
    // time parent walks via children list its lock should be held
    spin_lock(&dentry->lock);
    vfs_dentry* parent = dentry->parent;
    spin_unlock(&dentry->lock);

    // if destroy is called on initialisation failure - parent may be null
    if (parent && parent != dentry) {
        spin_lock(&parent->lock);
        linked_list_remove_node(&parent->children, &dentry->dentry_node);
        spin_unlock(&parent->lock);
    }

    vfs_inode_release(dentry->inode);
    strfree(dentry->name);
    kfree(dentry);

    spin_lock(&dcache_lock);
    vfs_dcache_deregister_allocation_unsafe();
    spin_unlock(&dcache_lock);

    // no need to release root dentry as it is already freed
    return parent != dentry ? parent : NULL;
}

// top level destroy function, should not be called inside other release/destroy
// functions or while holding dcache_lock, should be used when passed dentry is
// not in cache
static void vfs_dentry_destroy(vfs_dentry* dentry) {
    vfs_dentry_release(vfs_dentry_destroy_and_not_release_parent(dentry));
}

// this function should be called with dcache_lock held
static bool vfs_dentry_add_child(vfs_dentry* parent, vfs_dentry* child) {
    bool added = false;

    dcache_key key = {.parent = parent, .name = child->name};
    if (!dentry_cache_put(&dcache, key, child, NULL))
        goto out;

    spin_lock(&parent->lock);
    spin_lock(&child->lock);
    linked_list_add_last_node(&parent->children, &child->dentry_node);
    child->parent = parent;
    // child pins parent
    vfs_dentry_acquire(parent);
    added = true;

out:
    spin_unlock(&child->lock);
    spin_unlock(&parent->lock);
    return added;
}

static void vfs_dentry_remove_child(vfs_dentry* parent, vfs_dentry* child) {
    spin_lock(&dcache_lock);
    spin_lock(&parent->lock);
    spin_lock(&child->lock);

    child->detached = true;
    linked_list_remove_node(&parent->children, &child->dentry_node);
    child->parent = child;
    dcache_key key = {.parent = parent, .name = child->name};
    dentry_cache_remove(&dcache, key);

    spin_unlock(&child->lock);
    spin_unlock(&parent->lock);
    spin_unlock(&dcache_lock);

    // it is safe to break atomicity here, since both parent and child are
    // pinned by each other reference until this point
    vfs_dentry_release(parent);
}

void vfs_dentry_delete(vfs_dentry* dentry) {
    spin_lock(&dentry->lock);
    vfs_dentry* parent = dentry->parent;
    spin_unlock(&dentry->lock);

    // it is safe to use parent since caller is holding inode->mut
    vfs_dentry_remove_child(parent, dentry);
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

    memset(dentry, NULL, sizeof(vfs_dentry));

    dentry->name = name_copy;
    dentry->inode = inode;
    vfs_inode_acquire(inode);

    dentry->lock = SPIN_LOCK_STATIC_INITIALIZER;

    dentry->dying = false;
    dentry->detached = false;

    dentry->dentry_node = LINKED_LIST_NODE_OF(dentry);
    dentry->unused_node = LINKED_LIST_NODE_OF(dentry);
    dentry->children = LINKED_LIST_STATIC_INITIALIZER;

    dentry->refc = REF_COUNT_STATIC_INITIALIZER;

    vfs_dentry_acquire(dentry);
    return dentry;
}

vfs_dentry* vfs_dentry_create_root(vfs_inode* inode) {
    spin_lock(&dcache_lock);
    if (!vfs_dcache_register_allocation_unsafe()) {
        spin_unlock(&dcache_lock);
        return ERROR_PTR(-ENOMEM);
    }

    vfs_dentry* dentry = vfs_dentry_allocate(inode, "/");
    if (IS_ERROR(dentry)) {
        vfs_dcache_deregister_allocation_unsafe();
        goto out;
    }

    dentry->parent = dentry;

    dcache_key key = {.parent = dentry, .name = dentry->name};
    if (dentry_cache_put(&dcache, key, dentry, NULL))
        goto out;

    spin_unlock(&dcache_lock);
    vfs_dentry_destroy(dentry);
    return ERROR_PTR(-ENOMEM);

out:
    spin_unlock(&dcache_lock);
    return dentry;
}

vfs_dentry* vfs_dentry_create(vfs_dentry* parent, vfs_inode* inode,
                              string name) {

    dcache_key key = {.parent = parent, .name = name};

    spin_lock(&dcache_lock);
    vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry) {
        linked_list_remove_node(&unused_dentries, &dentry->unused_node);
        vfs_dentry_acquire(dentry);
        goto out;
    }

    bool allocate = vfs_dcache_register_allocation_unsafe();
    // need to recheck if dentry has not been added, since registration breaks
    // atomicity
    dentry = dentry_cache_get(&dcache, key);
    if (dentry) {
        if (allocate)
            vfs_dcache_deregister_allocation_unsafe();
        linked_list_remove_node(&unused_dentries, &dentry->unused_node);
        vfs_dentry_acquire(dentry);
        goto out;
    }

    dentry = vfs_dentry_allocate(inode, name);
    if (IS_ERROR(dentry)) {
        vfs_dcache_deregister_allocation_unsafe();
        goto out;
    }

    // we have allocated new dentry and must save it with copied name
    if (vfs_dentry_add_child(parent, dentry))
        goto out;

    vfs_dcache_deregister_allocation_unsafe();
    spin_unlock(&dcache_lock);
    vfs_dentry_destroy(dentry);
    return ERROR_PTR(-ENOMEM);

out:
    spin_unlock(&dcache_lock);
    return dentry;
}

vfs_dentry* vfs_dentry_get_parent(vfs_dentry* dentry) {
    spin_lock(&dentry->lock);
    vfs_dentry* parent = dentry->parent;
    ref_acquire(&parent->refc);
    spin_unlock(&dentry->lock);

    return parent;
}

void vfs_dentry_acquire(vfs_dentry* dentry) { ref_acquire(&dentry->refc); }

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

    spin_lock(&dentry->lock);
    bool destroy = dentry->refc.count == 1;

    // fast-path, release here only if we won't destroy dentry anyway
    if (!destroy)
        ref_release(&dentry->refc);
    spin_unlock(&dentry->lock);
    if (!destroy)
        return NULL;

    // slow-path, release and maybe destroy
    spin_lock(&dcache_lock);
    spin_lock(&dentry->lock);

    ref_release(&dentry->refc);
    if (dentry->refc.count != 0) {
        spin_unlock(&dentry->lock);
        spin_unlock(&dcache_lock);

        return NULL;
    }

    if (!dentry->detached && alive_dentries <= max_dentries) {
        linked_list_add_last_node(&unused_dentries, &dentry->unused_node);
        spin_unlock(&dentry->lock);
        spin_unlock(&dcache_lock);
        return NULL;
    }

    dcache_key key = {.parent = dentry->parent, .name = dentry->name};
    dentry_cache_remove(&dcache, key);
    dentry->dying = true;

    spin_unlock(&dentry->lock);
    spin_unlock(&dcache_lock);

    return vfs_dentry_destroy_and_not_release_parent(dentry);
}

// this function is based on vfs_dentry_release_and_not_release_parent and
// releasing dentry with possible chain of releases of all ancestors in a loop
void vfs_dentry_release(vfs_dentry* dentry) {
    vfs_dentry* parent = vfs_dentry_release_and_not_release_parent(dentry);

    while (parent) {
        parent = vfs_dentry_release_and_not_release_parent(parent);
    }
}

void vfs_dcache_init(u64 max_entries) {
    if (!dentry_cache_init(&dcache))
        panic("Can't init dentry cache");

    unused_dentries = LINKED_LIST_STATIC_INITIALIZER;
    alive_dentries = 0;
    max_dentries = max_entries;
}

static void vfs_dcache_maybe_shrink_unsafe() {
    while (alive_dentries >= max_dentries && unused_dentries.size != 0) {
        linked_list_node* unused_dentry_node =
            linked_list_remove_first_node(&unused_dentries);

        vfs_dentry* unused_dentry = unused_dentry_node->value;

        spin_lock(&unused_dentry->lock);
        unused_dentry->dying = true;
        vfs_dentry* parent = unused_dentry->parent;

        dcache_key key = {.parent = parent, .name = unused_dentry->name};
        dentry_cache_remove(&dcache, key);
        spin_unlock(&unused_dentry->lock);
        spin_unlock(&dcache_lock);

        // safe to forcefully destroy dentry since its no longer attached to
        // cache
        vfs_dentry_destroy(unused_dentry);
        spin_lock(&dcache_lock);
    }
}

// should be called with dcache_lock held, returns true when cache has room for
// new dentry, breaks atomicity by unlocking and locking dcache_lock while
// shrinking dcache
static bool vfs_dcache_register_allocation_unsafe() {
    vfs_dcache_maybe_shrink_unsafe();

    if (alive_dentries < max_dentries) {
        alive_dentries++;
        return true;
    }
    return false;
}

static void vfs_dcache_deregister_allocation_unsafe() { alive_dentries--; }

vfs_dentry* vfs_dcache_get(vfs_dentry* parent, string name) {
    dcache_key key = {.parent = parent, .name = name};

    spin_lock(&dcache_lock);
    vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry) {
        vfs_dentry_acquire(dentry);
        linked_list_remove_node(&unused_dentries, &dentry->unused_node);
    }
    spin_unlock(&dcache_lock);

    return dentry;
}