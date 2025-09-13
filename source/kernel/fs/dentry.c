#include "dentry.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
#include "../lib/string.h"

// for now - no references mean free the struct
// TODO: make cache smarter and free only when
// some limit of cached dentries is reached
typedef struct {
    struct vfs_dentry* parent;
    string name;
} dcache_key;

static u64 dcache_hash(dcache_key key) {
    return hash2((u64) key.parent, strhash(key.name));
}

static bool dcache_equals(dcache_key a, dcache_key b) {
    return a.parent == b.parent && streq(a.name, b.name);
}

DEFINE_HASH_TABLE(dentry_cache, dcache_key, struct vfs_dentry*, dcache_hash,
                  dcache_equals)

static lock dcache_lock = SPIN_LOCK_STATIC_INITIALIZER;

static dentry_cache dcache;

static void vfs_dentry_destroy(struct vfs_dentry* dentry) {
    // at this point dentry is unreachable for others
    if (dentry->parent != dentry)
        vfs_dentry_release(dentry->parent);

    vfs_inode_release(dentry->inode);
    strfree(dentry->name);
    kfree(dentry);
}

static struct vfs_dentry* vfs_dentry_allocate(struct vfs_dentry* parent,
                                              vfs_inode* inode, string name) {

    string name_copy = strcpy(name);
    if (!name_copy)
        return NULL;

    struct vfs_dentry* dentry = kmalloc(sizeof(struct vfs_dentry));
    if (!dentry) {
        strfree(name_copy);
        return ERROR_PTR(ENOMEM);
    }

    memset(dentry, NULL, sizeof(struct vfs_dentry));

    dentry->parent = (struct vfs_dentry*) (parent ? parent : dentry);
    dentry->inode = inode;
    dentry->name = parent ? name_copy : "/";
    dentry->lock = SPIN_LOCK_STATIC_INITIALIZER;
    dentry->refc = REF_COUNT_STATIC_INITIALIZER;

    if (parent)
        vfs_dentry_acquire(parent);

    vfs_inode_acquire(inode);
    vfs_dentry_acquire(dentry);
    return dentry;
}

struct vfs_dentry* vfs_dentry_create_root(vfs_inode* inode) {
    struct vfs_dentry* dentry = vfs_dentry_allocate(NULL, inode, "/");
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

struct vfs_dentry* vfs_dentry_create(struct vfs_dentry* parent,
                                     vfs_inode* inode, string name) {

    dcache_key key = {.parent = parent, .name = name};

    spin_lock(&dcache_lock);
    struct vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry) {
        vfs_dentry_acquire(dentry);
        goto out;
    }

    dentry = vfs_dentry_allocate(parent, inode, name);
    if (IS_ERROR(dentry))
        goto out;

    if (dentry_cache_put(&dcache, key, dentry, NULL))
        goto out;

    spin_unlock(&dcache_lock);
    vfs_dentry_destroy(dentry);
    return ERROR_PTR(-ENOMEM);

out:
    spin_unlock(&dcache_lock);
    return dentry;
}

struct vfs_dentry* vfs_dentry_get_parent(struct vfs_dentry* dentry) {
    spin_lock(&dentry->lock);
    struct vfs_dentry* parent = dentry->parent;
    ref_acquire(&parent->refc);
    spin_unlock(&dentry->lock);

    return parent;
}

void vfs_dentry_acquire(struct vfs_dentry* dentry) {
    ref_acquire(&dentry->refc);
}

void vfs_dentry_release(struct vfs_dentry* dentry) {
    spin_lock(&dentry->lock);
    ref_release(&dentry->refc);
    bool destroy = dentry->refc.count == 0;
    spin_unlock(&dentry->lock);
    if (!destroy)
        return;

    spin_lock(&dcache_lock);
    spin_lock(&dentry->lock);
    if ((destroy = dentry->refc.count == 0)) {
        dcache_key key = {.parent = dentry->parent, .name = dentry->name};
        dentry_cache_remove(&dcache, key);
    }
    spin_unlock(&dcache_lock);
    if (!destroy)
        return;

    vfs_dentry_destroy(dentry);
}

void vfs_dcache_init() {
    if (!dentry_cache_init(&dcache))
        panic("Can't init dentry cache");
}

struct vfs_dentry* vfs_dcache_get(struct vfs_dentry* parent, string name) {
    dcache_key key = {.parent = parent, .name = name};

    spin_lock(&dcache_lock);
    struct vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry)
        vfs_dentry_acquire(dentry);
    spin_unlock(&dcache_lock);

    return dentry;
}