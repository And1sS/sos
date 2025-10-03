#include "dentry.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/hash.h"
#include "../../lib/string.h"

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

    memset(dentry, NULL, sizeof(vfs_dentry));

    dentry->name = name_copy;
    dentry->inode = inode;
    vfs_inode_acquire(inode);

    dentry->lock = SPIN_LOCK_STATIC_INITIALIZER;

    dentry->dying = false;
    dentry->detached = false;
    dentry->hashed = false;

    dentry->dentry_node = LINKED_LIST_NODE_OF(dentry);
    dentry->children = LINKED_LIST_STATIC_INITIALIZER;

    dentry->unused_node = LINKED_LIST_NODE_OF(dentry);

    dentry->refc = REF_COUNT_STATIC_INITIALIZER;

    vfs_dentry_acquire(dentry);
    return dentry;
}

vfs_dentry* vfs_dentry_create_root(vfs_inode* inode) {
    spin_lock(&dcache_lock);
    vfs_dentry* dentry = ERROR_PTR(-ENOMEM);
    if (!vfs_dcache_register_allocation_unsafe())
        goto out;

    dentry = vfs_dentry_allocate(inode, "/");
    if (IS_ERROR(dentry)) {
        vfs_dcache_deregister_allocation_unsafe();
        goto out;
    }

    dentry->parent = dentry;
    vfs_dcache_rehash(dentry);
    spin_unlock(&dcache_lock);

out:
    spin_unlock(&dcache_lock);
    return dentry;
}

vfs_dentry* vfs_dentry_create(vfs_dentry* parent, vfs_inode* inode,
                              string name) {

    spin_lock(&dcache_lock);
    vfs_dentry* dentry = vfs_dcache_lookup(parent, name);
    if (dentry) {
        linked_list_remove_node(&unused_dentries, &dentry->unused_node);
        vfs_dentry_acquire(dentry);
        goto out;
    }

    bool allocate = vfs_dcache_register_allocation_unsafe();

    // need to recheck if dentry has not been added, since registration breaks
    // atomicity
    if ((dentry = vfs_dcache_lookup(parent, name))) {
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

    vfs_dentry_acquire(parent);
    dentry->parent = parent;
    vfs_dcache_put(dentry);

    vfs_dentry_acquire(parent);

    spin_lock(&parent->lock);
    linked_list_add_last_node(&parent->children, &dentry->dentry_node);
    spin_unlock(&parent->lock);

out:
    spin_unlock(&dcache_lock);
    return dentry;
}

u64 vfs_dentry_move(vfs_dentry* new_parent, vfs_dentry* child, string name) {
    // Safe to do plain read of parent since caller already holds old parents
    // rw_mutex so no concurrent moves can be done and visibility was carried
    // either via dcache when lookup occurred or later via scheduler if after
    // lookup cpu has been changed
    vfs_dentry* old_parent = child->parent;

    // remove old link and point child towards new parent
    spin_lock(&old_parent->lock);
    linked_list_remove_node(&old_parent->children, &child->dentry_node);
    spin_unlock(&old_parent->lock);

    // replace parent reference and rehash child
    spin_lock(&dcache_lock);
    vfs_dentry* existing_child = vfs_dcache_lookup(new_parent, name);
    spin_lock(&existing_child->lock);
    if (existing_child)
        vfs_dcache_unhash(existing_child);
    spin_unlock(&existing_child->lock);

    spin_lock(&child->lock);
    vfs_dcache_unhash(child);
    child->name = name;
    child->parent = new_parent;
    vfs_dcache_put(child);
    if (!existing_child)
        vfs_dentry_acquire(new_parent);
    spin_unlock(&child->lock);
    spin_unlock(&dcache_lock);

    if (existing_child) {
        // remove dentry we have replaced from parent
        spin_lock(&new_parent->lock);
        linked_list_remove_node(&new_parent->children,
                                &existing_child->dentry_node);
        spin_unlock(&new_parent->lock);

        // detach replaced dentry from its old parent
        spin_lock(&existing_child->lock);
        existing_child->parent = existing_child;
        existing_child->detached = true;
        spin_unlock(&existing_child->lock);
    }

    vfs_dentry_release(old_parent);
    // no need to release new_parent since if we replaced some dentry then
    // parent reference just been moved to new child;

    return 0;
}

void vfs_dentry_delete(vfs_dentry* dentry) {
    vfs_dentry* parent = vfs_dentry_get_parent(dentry);

    // it is safe to use parent since caller is holding inode->mut
    spin_lock(&dcache_lock);
    spin_lock(&dentry->lock);

    vfs_dcache_unhash(dentry);
    dentry->detached = true;
    dentry->parent = dentry;

    spin_unlock(&dentry->lock);
    spin_unlock(&dcache_lock);

    vfs_dentry_release(parent);
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

    // fast-path, release here only if we won't destroy dentry anyway
    spin_lock(&dentry->lock);
    bool destroy = dentry->refc.count == 1;
    if (!destroy)
        ref_release(&dentry->refc);
    spin_unlock(&dentry->lock);
    if (!destroy)
        return NULL;

    // slow-path, release and maybe destroy
    spin_lock(&dcache_lock);
    spin_lock(&dentry->lock);

    vfs_dentry* parent = NULL;
    ref_release(&dentry->refc);
    if (dentry->refc.count != 0)
        goto out;

    if (!dentry->detached && alive_dentries <= max_dentries) {
        linked_list_add_last_node(&unused_dentries, &dentry->unused_node);
        goto out;
    }

    dentry->dying = true;
    vfs_dcache_unhash(dentry);

    spin_unlock(&dentry->lock);
    spin_unlock(&dcache_lock);

    return vfs_dentry_destroy_and_not_release_parent(dentry);

out:
    spin_unlock(&dentry->lock);
    spin_unlock(&dcache_lock);

    return parent;
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
        vfs_dcache_unhash(unused_dentry);
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
    spin_lock(&dcache_lock);
    vfs_dentry* dentry = vfs_dcache_lookup(parent, name);
    if (dentry) {
        vfs_dentry_acquire(dentry);
        linked_list_remove_node(&unused_dentries, &dentry->unused_node);
    }
    spin_unlock(&dcache_lock);

    return dentry;
}