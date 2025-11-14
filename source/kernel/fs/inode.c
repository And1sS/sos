#include "inode.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/flagops.h"
#include "../lib/hash.h"
#include "../lib/math.h"
#include "super_block.h"

// for now - no references mean free the struct
// TODO: make cache smarter and free only when
// some limit of cached dentries is reached
typedef struct {
    struct vfs_super_block* sb;
    u64 inode_id;
} icache_key;

static u64 icache_hash(icache_key key) {
    return hash2(key.sb->id, key.inode_id);
}

static bool icache_comparator(icache_key a, icache_key b) {
    return a.inode_id == b.inode_id && a.sb->id == b.sb->id;
}

DEFINE_HASH_TABLE(inode_cache, icache_key, vfs_inode*, icache_hash,
                  icache_comparator)

static lock icache_lock = SPIN_LOCK_STATIC_INITIALIZER;

static inode_cache icache;

static volatile u64 inodes_cached = 0;
static u64 max_cached;

void vfs_icache_init(u64 max_inodes) {
    max_cached = max_inodes;
    if (!inode_cache_init(&icache))
        panic("Can't init inode cache");
}

static void vfs_inode_destroy(vfs_inode* inode) {
    // at this point inode is unreachable for others

    // safe to do plain links read since no one can modify this and visibility
    // is carried by refc
    if (inode->flags & INODE_INITIALIZED && inode->links == 0
        && inode->ops->evict)
        inode->ops->evict(inode);

    vfs_super_release(inode->sb);
    kfree(inode);
}

static vfs_inode* vfs_inode_allocate(u64 id, vfs_super_block* sb) {
    vfs_inode* inode = kmalloc(sizeof(vfs_inode));
    if (!inode)
        return (vfs_inode*) ERROR_PTR(-ENOMEM);

    memset(inode, 0, sizeof(vfs_inode));

    inode->id = id;
    inode->sb = sb;

    inode->mut = RW_MUTEX_STATIC_INITIALIZER;

    inode->flags = 0;
    inode->links = 0;
    inode->refc = 0;

    inode->lock = SPIN_LOCK_STATIC_INITIALIZER;

    vfs_super_acquire(sb);

    return vfs_inode_acquire(inode);
}

static bool vfs_inode_initialization_finished(vfs_inode* inode) {
    return (inode->flags & INODE_INITIALIZED)
           || (inode->flags & INODE_INITIALIZATION_FAILED);
}

static bool vfs_inode_await_initialization(vfs_inode* inode) {
    if (TEST_FLAG(inode->flags, INODE_INITIALIZED))
        return true;

    spin_lock(&inode->lock);
    CON_VAR_WAIT_FOR(&inode->initialization_cvar, &inode->lock,
                     vfs_inode_initialization_finished(inode));
    spin_unlock(&inode->lock);

    return TEST_FLAG(inode->flags, INODE_INITIALIZED);
}

vfs_inode* vfs_icache_get(vfs_super_block* sb, u64 id) {
    icache_key key = {.sb = sb, .inode_id = id};

retry:
    spin_lock(&icache_lock);
    vfs_inode* inode = inode_cache_get(&icache, key);
    if (inode) {
        vfs_inode_acquire(inode);
        spin_unlock(&icache_lock);

        if (!vfs_inode_await_initialization(inode)) {
            vfs_inode_release(inode);
            goto retry;
        }

        return inode;
    }

    inode = vfs_inode_allocate(id, sb);
    if (IS_ERROR(inode))
        goto out;

    if (inode_cache_put(&icache, key, inode, NULL))
        goto out;

    spin_unlock(&icache_lock);
    vfs_inode_destroy(inode);
    return ERROR_PTR(-ENOMEM);

out:
    spin_unlock(&icache_lock);
    return inode;
}

void vfs_inode_drop(vfs_inode* inode) {
    spin_lock(&icache_lock);
    icache_key key = {.sb = inode->sb, .inode_id = inode->id};
    inode_cache_remove(&icache, key);
    spin_unlock(&icache_lock);

    vfs_inode_destroy(inode);
}

vfs_inode* vfs_inode_unlock_new(vfs_inode* inode) {
    spin_lock(&inode->lock);
    inode->flags |= INODE_INITIALIZED;
    con_var_broadcast(&inode->initialization_cvar);
    spin_unlock(&inode->lock);

    return inode;
}

void vfs_inode_unlock_failed(vfs_inode* inode) {
    spin_lock(&icache_lock);
    icache_key key = {.sb = inode->sb, .inode_id = inode->id};
    inode_cache_remove(&icache, key);
    spin_unlock(&icache_lock);

    spin_lock(&inode->lock);
    inode->flags |= INODE_INITIALIZATION_FAILED;
    con_var_broadcast(&inode->initialization_cvar);
    spin_unlock(&inode->lock);
}

vfs_inode* vfs_inode_acquire(vfs_inode* inode) {
    atomic_increment(&inode->refc);
    return inode;
}

void vfs_inode_release(vfs_inode* inode) {
    spin_lock(&icache_lock);
    if (atomic_decrement_and_get(&inode->refc) != 0) {
        spin_unlock(&icache_lock);
        return;
    }

    icache_key key = {.sb = inode->sb, .inode_id = inode->id};
    inode_cache_remove(&icache, key);
    spin_unlock(&icache_lock);

    vfs_inode_destroy(inode);
}

void vfs_inode_lock_shared(vfs_inode* inode) {
    rw_mutex_lock_read(&inode->mut);
}

void vfs_inode_unlock_shared(vfs_inode* inode) {
    rw_mutex_unlock_read(&inode->mut);
}

void vfs_inode_lock(vfs_inode* inode) { rw_mutex_lock_write(&inode->mut); }

void vfs_inode_unlock(vfs_inode* inode) { rw_mutex_unlock_write(&inode->mut); }

void vfs_inodes_lock(vfs_inode* left, vfs_inode* right) {
    vfs_inode_lock(MIN(left, right));
    vfs_inode_lock(MAX(left, right));
}

void vfs_inodes_unlock(vfs_inode* left, vfs_inode* right) {
    vfs_inode_unlock(left);
    vfs_inode_unlock(right);
}

void vfs_inode_drop_link(vfs_inode* inode) { atomic_decrement(&inode->links); }

void vfs_inode_add_link(vfs_inode* inode) { atomic_increment(&inode->links); }
