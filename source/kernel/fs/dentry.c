#include "dentry.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
#include "../lib/string.h"

// for now - no references mean free the struct
// TODO: make cache smarter and free only when
// some limit of cached dentries is reached
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

static void vfs_dentry_destroy(vfs_dentry* dentry) {
    // at this point dentry is unreachable for others
    spin_lock(&dentry->lock);
    vfs_dentry* parent = dentry->parent;
    spin_unlock(&dentry->lock);

    if (parent != dentry) {
        spin_lock(&parent->lock);
        linked_list_remove_node(&parent->children, &dentry->dentry_node);
        spin_unlock(&parent->lock);
        vfs_dentry_release(parent);
    }

    vfs_inode_release(dentry->inode);
    strfree(dentry->name);
    kfree(dentry);
}

// this function should be called with dcache_lock held
static bool vfs_dentry_add_child(vfs_dentry* parent, vfs_dentry* child) {
    bool added = false;

    // child pins parent
    vfs_dentry_acquire(parent);

    dcache_key key = {.parent = parent, .name = child->name};
    if (!dentry_cache_put(&dcache, key, child, NULL))
        goto out;

    spin_lock(&parent->lock);
    spin_lock(&child->lock);
    linked_list_add_last_node(&parent->children, &child->dentry_node);
    child->parent = parent;
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

    linked_list_remove_node(&parent->children, &child->dentry_node);
    child->parent = child;
    dcache_key key = {.parent = parent, .name = child->name};
    dentry_cache_remove(&dcache, key);

    spin_unlock(&parent->lock);
    spin_unlock(&child->lock);
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

static vfs_dentry* vfs_dentry_allocate(vfs_dentry* parent, vfs_inode* inode,
                                       string name) {

    string name_copy = strcpy(parent ? name : "/");
    if (!name_copy)
        return ERROR_PTR(-ENOMEM);

    vfs_dentry* dentry = kmalloc(sizeof(struct vfs_dentry));
    if (!dentry) {
        strfree(name_copy);
        return ERROR_PTR(-ENOMEM);
    }

    memset(dentry, NULL, sizeof(struct vfs_dentry));

    dentry->name = name_copy;
    dentry->inode = inode;
    vfs_inode_acquire(inode);

    dentry->lock = SPIN_LOCK_STATIC_INITIALIZER;

    dentry->dead = true;

    dentry->dentry_node = LINKED_LIST_NODE_OF(dentry);
    dentry->children = LINKED_LIST_STATIC_INITIALIZER;

    dentry->refc = REF_COUNT_STATIC_INITIALIZER;

    vfs_dentry_acquire(dentry);
    return dentry;
}

vfs_dentry* vfs_dentry_create_root(vfs_inode* inode) {
    vfs_dentry* dentry = vfs_dentry_allocate(NULL, inode, "/");
    if (IS_ERROR(dentry))
        return dentry;

    dentry->parent = dentry;

    spin_lock(&dcache_lock);
    dcache_key key = {.parent = dentry, .name = dentry->name};
    bool added = dentry_cache_put(&dcache, key, dentry, NULL);
    spin_unlock(&dcache_lock);

    if (added)
        return dentry;

    vfs_dentry_destroy(dentry);
    return ERROR_PTR(-ENOMEM);
}

vfs_dentry* vfs_dentry_create(vfs_dentry* parent, vfs_inode* inode,
                              string name) {

    dcache_key key = {.parent = parent, .name = name};

    spin_lock(&dcache_lock);
    vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry) {
        vfs_dentry_acquire(dentry);
        goto out;
    }

    dentry = vfs_dentry_allocate(parent, inode, name);
    if (IS_ERROR(dentry))
        goto out;

    // we have allocated new dentry and must save it with copied name
    if (vfs_dentry_add_child(parent, dentry))
        goto out;

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

void vfs_dentry_release(vfs_dentry* dentry) {
    spin_lock(&dentry->lock);
    ref_release(&dentry->refc);
    bool destroy = dentry->refc.count == 0;
    spin_unlock(&dentry->lock);
    if (!destroy)
        return;

    spin_lock(&dcache_lock);
    spin_lock(&dentry->lock);
    if ((destroy = dentry->refc.count == 0)) {
        dentry->dead = true;
        dcache_key key = {.parent = dentry->parent, .name = dentry->name};
        dentry_cache_remove(&dcache, key);
    }
    spin_unlock(&dentry->lock);
    spin_unlock(&dcache_lock);
    if (!destroy)
        return;

    vfs_dentry_destroy(dentry);
}

void vfs_dcache_init() {
    if (!dentry_cache_init(&dcache))
        panic("Can't init dentry cache");
}

vfs_dentry* vfs_dcache_get(vfs_dentry* parent, string name) {
    dcache_key key = {.parent = parent, .name = name};

    spin_lock(&dcache_lock);
    vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry)
        vfs_dentry_acquire(dentry);
    spin_unlock(&dcache_lock);

    return dentry;
}