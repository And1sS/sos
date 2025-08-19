#include "icache.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"

typedef struct {
    vfs_super_block* fs;
    u64 inode_id;
} icache_key;

static u64 icache_hash(icache_key key) {
    return hash2(key.fs->id, key.inode_id);
}

static bool icache_comparator(icache_key a, icache_key b) {
    return a.inode_id == b.inode_id && a.fs->id == b.fs->id;
}

DEFINE_HASH_TABLE(inode_cache, icache_key, vfs_inode*, icache_hash,
                  icache_comparator)

static lock icache_lock = SPIN_LOCK_STATIC_INITIALIZER;

static inode_cache icache;

static volatile u64 inodes_cached = 0;
static u64 max_cached;

bool vfs_icache_init(u64 max_inodes) {
    max_cached = max_inodes;
    return inode_cache_init(&icache);
}

static vfs_inode* inode_create(vfs_super_block* fs, u64 id) {
    vfs_inode* inode = kmalloc(sizeof(vfs_inode));
    if (!inode)
        return (vfs_inode*) -ENOMEM;

    return inode;
}

vfs_inode* vfs_icache_get(vfs_super_block* sb, u64 id) {
    icache_key key = {.fs = sb, .inode_id = id};

    // TODO: this one should not disable irq, but only preemption
    bool interrupts_enabled = spin_lock_irq_save(&icache_lock);
    vfs_inode* inode = inode_cache_get(&icache, key);
    if (!inode) {
        inode = inode_create(sb, id);
    }

    if (IS_ERROR(inode)) {
        goto out;
    }

    // TODO: acquire sb reference
    vfs_inode_acquire(inode);

out:
    spin_unlock_irq_restore(&icache_lock, interrupts_enabled);
    return inode;
}
