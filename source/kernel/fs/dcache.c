#include "dcache.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
#include "../lib/string.h"

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

static u64 dcache_entries = 0;

/*
 * Dcache locking order:
 * 1) dcache_lock
 * 2) parent dentry lock
 * 3) child dentry lock
 */
static lock dcache_lock = SPIN_LOCK_STATIC_INITIALIZER;

static dentry_cache dcache;

static void dcache_remove_unsafe(struct vfs_dentry* dentry);
static void vfs_dentry_destroy(struct vfs_dentry* dentry);

static struct vfs_dentry* vfs_dentry_allocate(struct vfs_dentry* parent,
                                              vfs_inode* inode, string name) {

    string name_copy = strcpy(name);
    if (!name_copy)
        return NULL;

    struct vfs_dentry* new = kmalloc(sizeof(struct vfs_dentry));
    if (!new) {
        strfree(name_copy);
        return new;
    }

    memset(new, NULL, sizeof(struct vfs_dentry));

    new->parent = (struct vfs_dentry*) (parent ? parent : new);
    new->inode = inode;
    new->name = parent ? name_copy : "/";
    new->lock = SPIN_LOCK_STATIC_INITIALIZER;
    new->refc = REF_COUNT_STATIC_INITIALIZER;

    if (parent) {
        vfs_dentry_acquire(parent);
    }
    vfs_inode_acquire(inode);
    vfs_dentry_acquire(new);
    return new;
}

struct vfs_dentry* vfs_dentry_create_root(vfs_inode* inode) {
    struct vfs_dentry* root = vfs_dentry_allocate(NULL, inode, "/");
    if (!root)
        return ERROR_PTR(-ENOMEM);

    root->parent = root;

    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);

    dcache_key key = {.parent = root, .name = root->name};
    bool added = dentry_cache_put(&dcache, key, root, NULL);
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

    if (!added) {
        vfs_dentry_destroy(root);
        return ERROR_PTR(-ENOMEM);
    }

    return root;
}

struct vfs_dentry* vfs_dentry_create(struct vfs_dentry* parent,
                                     vfs_inode* inode, string name) {

    struct vfs_dentry* new = vfs_dentry_allocate(parent, inode, name);
    if (!new)
        return ERROR_PTR(-ENOMEM);

    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);
    dcache_key key = {.parent = parent, .name = new->name};
    struct vfs_dentry* old = dentry_cache_get(&dcache, key);
    bool added = !old && dentry_cache_put(&dcache, key, new, NULL);
    // TODO: acquire old dentry reference
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

    if (!added) {
        vfs_dentry_destroy(new);
        return old;
    }

    return new;
}

struct vfs_dentry* vfs_dentry_acquire_parent(struct vfs_dentry* dentry) {
    bool interrupts_enabled = spin_lock_irq_save(&dentry->lock);
    struct vfs_dentry* parent = dentry->parent;
    ref_acquire(&parent->refc);
    spin_unlock_irq_restore(&parent->lock, interrupts_enabled);

    return parent;
}

void vfs_dentry_destroy(struct vfs_dentry* dentry) {
    if (dentry == dentry->parent)
        panic("Trying to destroy sb root");

    if (dentry->parent != dentry)
        vfs_dentry_release(dentry->parent);

    vfs_inode_release(dentry->inode);
    strfree(dentry->name);
    kfree(dentry);
}

void vfs_dentry_acquire(struct vfs_dentry* dentry) {
    bool interrupts_enabled = spin_lock_irq_save(&dentry->lock);
    ref_acquire(&dentry->refc);
    spin_unlock_irq_restore(&dentry->lock, interrupts_enabled);
}

void vfs_dentry_release(struct vfs_dentry* dentry) {
    bool interrupts_enabled = spin_lock_irq_save(&dentry->lock);
    ref_release(&dentry->refc);
    u64 count = dentry->refc.count;
    spin_unlock_irq_restore(&dentry->lock, interrupts_enabled);
    if (count != 0)
        return;

    interrupts_enabled = spin_lock_irq_save(&dcache_lock);
    spin_lock(&dentry->lock);
    count = dentry->refc.count;
    if (count == 0)
        dcache_remove_unsafe(dentry);
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

    vfs_dentry_destroy(dentry);
}

void vfs_dcache_init() { dentry_cache_init(&dcache); }

struct vfs_dentry* vfs_dcache_get(struct vfs_dentry* parent, string name) {
    dcache_key key = {.parent = parent, .name = name};

    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);
    struct vfs_dentry* dentry = dentry_cache_get(&dcache, key);
    if (dentry)
        vfs_dentry_acquire(dentry);
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

    return dentry;
}

void dcache_remove_unsafe(struct vfs_dentry* dentry) {
    dcache_key key = {.parent = dentry->parent, .name = dentry->name};
    dentry_cache_remove(&dcache, key);
}

void dcache_remove(struct vfs_dentry* dentry) {
    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);
    dcache_remove_unsafe(dentry);
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);
}