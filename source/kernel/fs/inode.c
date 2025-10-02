#include "inode.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
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
    inode->initialised = false;
    inode->dead = false;

    inode->mut = RW_MUTEX_STATIC_INITIALIZER;

    inode->lock = SPIN_LOCK_STATIC_INITIALIZER;
    inode->refc = REF_COUNT_STATIC_INITIALIZER;

    vfs_super_acquire(sb);
    vfs_inode_acquire(inode);

    return inode;
}

vfs_inode* vfs_icache_get(vfs_super_block* sb, u64 id) {
    icache_key key = {.sb = sb, .inode_id = id};

    spin_lock(&icache_lock);
    vfs_inode* inode = inode_cache_get(&icache, key);
    if (inode) {
        vfs_inode_acquire(inode);
        spin_unlock(&icache_lock);

        vfs_inode_await_initialization(inode);
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

void vfs_inode_await_initialization(vfs_inode* inode) {
    spin_lock(&inode->lock);
    while (!inode->initialised) {
        CON_VAR_WAIT_FOR(&inode->initialization_cvar, &inode->lock,
                         inode->initialised);
    }
    spin_unlock(&inode->lock);
}

void vfs_inode_unlock_new(vfs_inode* inode) {
    spin_lock(&inode->lock);
    inode->initialised = true;
    con_var_broadcast(&inode->initialization_cvar);
    spin_unlock(&inode->lock);
}

void vfs_inode_acquire(vfs_inode* inode) {
    spin_lock(&inode->lock);
    ref_acquire(&inode->refc);
    spin_unlock(&inode->lock);
}

void vfs_inode_release(vfs_inode* inode) {
    spin_lock(&inode->lock);
    ref_release(&inode->refc);
    bool destroy = inode->refc.count == 0;
    spin_unlock(&inode->lock);
    if (!destroy)
        return;

    spin_lock(&icache_lock);
    spin_lock(&inode->lock);
    if ((destroy = inode->refc.count == 0)) {
        icache_key key = {.sb = inode->sb, .inode_id = inode->id};
        inode_cache_remove(&icache, key);
    }
    spin_unlock(&icache_lock);

    if (destroy)
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
    vfs_inode_unlock(MAX(left, right));
    vfs_inode_unlock(MIN(left, right));
}
void vfs_inode_drop_link(vfs_inode* inode) { UNUSED(inode); }
