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

static vfs_inode* vfs_inode_allocate(u64 id, struct vfs_super_block* sb) {
    vfs_inode* inode = kmalloc(sizeof(vfs_inode));
    if (!inode)
        return (vfs_inode*) ERROR_PTR(-ENOMEM);

    memset(inode, 0, sizeof(vfs_inode));

    inode->id = id;
    inode->sb = sb;
    inode->initialised = false;
    inode->lock = SPIN_LOCK_STATIC_INITIALIZER;
    inode->refc = REF_COUNT_STATIC_INITIALIZER;
    inode->aliasc = REF_COUNT_STATIC_INITIALIZER;

    vfs_super_acquire(sb);
    vfs_inode_acquire(inode);

    return inode;
}

vfs_inode* vfs_icache_get(struct vfs_super_block* sb, u64 id) {
    icache_key key = {.sb = sb, .inode_id = id};

    spin_lock(&icache_lock);
    vfs_inode* inode = inode_cache_get(&icache, key);
    if (inode) {
        vfs_inode_acquire(inode);
        goto out;
    }

    inode = vfs_inode_allocate(id, sb);
    if (IS_ERROR(inode))
        goto out;

    if (!inode_cache_put(&icache, key, inode, NULL)) {
        vfs_inode_destroy(inode);
        inode = ERROR_PTR(-ENOMEM);
    }

out:
    spin_unlock(&icache_lock);

    return inode;
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
