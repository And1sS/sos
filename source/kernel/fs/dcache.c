#include "dcache.h"
#include "../lib/string.h"
#include "../memory/heap/kheap.h"
#include "inode.h"

DEFINE_HASH_TABLE_NO_MM(dirtable, string, vfs_inode*, strhash, strcmp)

static bool inode_equals(vfs_inode* a, vfs_inode* b) {
    return a->id == b->id && a->fs->id == b->fs->id;
}

static u64 inode_hash(vfs_inode* inode) {
    return ((inode->fs->id << 5) + inode->id) << 5;
}
DEFINE_HASH_TABLE_NO_MM(dcache_table, vfs_inode*, dirtable*, inode_hash,
                        inode_equals)
static dcache_table dcache;
static lock dcache_lock = SPIN_LOCK_STATIC_INITIALIZER;

void dcache_init() { dcache_table_init(&dcache); }

vfs_inode* dcache_get(vfs_inode* dir, string name) {
    vfs_inode* result = NULL;

    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);
    dirtable* entry = dcache_table_get(&dcache, dir);
    if (!entry)
        goto out;

    result = dirtable_get(entry, name);
    if (result)
        vfs_inode_acquire(result);
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

out:
    return result;
}

static bool dcache_put_exactly(vfs_inode* dir, string name, vfs_inode* child) {

    bool result = false;
    dirtable* entry = dcache_table_get(&dcache, dir);
    entry = entry ? entry : dirtable_create();
    if (!entry)
        return false;

    if (!dirtable_get(entry, name)) {
        result = dirtable_put(entry, name, child, NULL);
    }
    if (!entry->size) {
        dcache_table_remove(&dcache, dir);
        dirtable_destroy(entry);
    }
    return result;
}

static void dcache_remove_exactly(vfs_inode* dir, string name) {
    dirtable* entry = dcache_table_get(&dcache, dir);
    dirtable_remove(entry, name);

    if (!entry->size) {
        dcache_table_remove(&dcache, dir);
        dirtable_destroy(entry);
    }
}

bool dcache_put(vfs_inode* dir, string name, vfs_inode* child) {
    bool result = false;

    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);

    if (!dcache_put_exactly(dir, name, child))
        goto out;

    if (!dcache_put_exactly(child, VFS_PARENT_NAME, dir)) {
        dcache_remove_exactly(dir, name);
        goto out;
    }

    result = true;

out:
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

    return result;
}

void dcache_remove(vfs_inode* dir, string name) {
    bool interrupts_enabled = spin_lock_irq_save(&dcache_lock);

    vfs_inode* subdir = dcache_get(dir, name);
    if (!subdir)
        panic("Removing NULL subdir");

    dcache_remove_exactly(dir, name);
    dcache_remove_exactly(subdir, VFS_PARENT_NAME);

    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);
}