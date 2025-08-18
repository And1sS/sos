#include "dcache.h"
#include "../lib/string.h"
#include "../memory/heap/kheap.h"
#include "inode.h"

static bool inode_equals(vfs_inode* a, vfs_inode* b) {
    return a->id == b->id && a->fs->id == b->fs->id;
}

static u64 inode_hash(vfs_inode* inode) {
    return ((inode->fs->id << 5) + inode->id) << 5;
}
DEFINE_HASH_TABLE(dirtable, string, vfs_inode*, strhash, streq)
DEFINE_HASH_TABLE(dcache_table, vfs_inode*, dirtable*, inode_hash,
                        inode_equals)

static u64 dcache_entries;
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

out:
    spin_unlock_irq_restore(&dcache_lock, interrupts_enabled);

    return result;
}

static bool dcache_put_exactly(vfs_inode* dir, string name, vfs_inode* child) {
    bool result = false;
    dirtable* entry = dcache_table_get(&dcache, dir);
    if (!entry) {
        entry = dirtable_create();
        if (!entry || !dcache_table_put(&dcache, dir, entry, NULL)) {
            dirtable_destroy(entry);
            return false;
        }
    }

    if (!dirtable_get(entry, name)) {
        // TODO: handle string copying properly
        result = dirtable_put(entry, strcpy(name), child, NULL);
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
    kfree((void*) name);

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

// typedef struct _tree_node {
//     string name;
//     u64 id;
//
//     struct _tree_node* parent;
//     array_list subnodes;
// } tree_node;
//
// void dirtable_print1(dirtable* table) {
//     print("dirtable { ");
//     for (unsigned long i = 0; i < table->buckets_num; i++) {
//         linked_list* bucket = &table->buckets[i];
//         linked_list_node* entry_node = bucket->head;
//         while (entry_node != 0) {
//             dirtable_entry* entry = (dirtable_entry*) entry_node->value;
//             print("path = \"");
//             print( entry->key);
//             print("\", node = \"");
//             print(((tree_node*) entry->value->private_data)->name);
//             print("\"; ");
//             entry_node = entry_node->next;
//         }
//     }
//     print("}");
// }
//
// void dcache_table_print1(dcache_table* table) {
//     println("hash table {");
//     print("size = ");
//     print_u64(table->size);
//     println("");
//     for (unsigned long i = 0; i < table->buckets_num; i++) {
//         linked_list* bucket = &table->buckets[i];
//         linked_list_node* entry_node = bucket->head;
//         while (entry_node != 0) {
//             dcache_table_entry* entry = (dcache_table_entry*)
//             entry_node->value; print("    key = \""); print( ((tree_node*)
//             entry->key->private_data)->name); print("\", value = ");
//             dirtable_print1(entry->value);
//             println("");
//             entry_node = entry_node->next;
//         }
//     }
//     println("}");
// }