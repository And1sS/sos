#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
#include "inode.h"
#include "super_block.h"

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

static vfs_inode* vfs_inode_create(u64 id, struct vfs_super_block* sb) {
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

    return inode;
}

vfs_inode* vfs_icache_get(struct vfs_super_block* sb, u64 id) {
    icache_key key = {.sb = sb, .inode_id = id};

    // TODO: this one should not disable irq, but only preemption
    bool interrupts_enabled = spin_lock_irq_save(&icache_lock);
    vfs_inode* inode = inode_cache_get(&icache, key);
    if (!inode) {
        inode = vfs_inode_create(id, sb);
    }

    if (IS_ERROR(inode))
        goto out;

    vfs_inode_acquire(inode);

out:
    spin_unlock_irq_restore(&icache_lock, interrupts_enabled);

    if (!IS_ERROR(inode))
        vfs_super_acquire(sb);

    return inode;
}

void vfs_inode_acquire(vfs_inode* inode) {
    bool interrupts_enabled = spin_lock_irq_save(&inode->lock);
    ref_acquire(&inode->refc);
    spin_unlock_irq_restore(&inode->lock, interrupts_enabled);
}

void vfs_inode_release(vfs_inode* inode) {
    bool interrupts_enabled = spin_lock_irq_save(&inode->lock);
    ref_release(&inode->refc);
    spin_unlock_irq_restore(&inode->lock, interrupts_enabled);
}
