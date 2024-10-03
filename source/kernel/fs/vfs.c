#include "vfs.h"
#include "../error/errno.h"
#include "../lib/container/hash_table/hash_table.h"
#include "../synchronization/spin_lock.h"
#include <string.h>

static lock registered_filesystems_lock;
static hash_table registered_filesystems; // fs name -> fs

static lock mounted_filesystems_lock;
static hash_table mounted_filesystems; // dentry -> fs

u64 hash_string(string str) {
    u64 hash = 17;
    for (u64 i = 0; str[i] != '\0'; i++) {
        hash = 17 * hash + str[i];
    }

    return hash;
}

void vfs_register_fs(vfs_fs_type* fs) {
    bool interrupts_enabled = spin_lock_irq_save(&registered_filesystems_lock);
    hash_table_put(&registered_filesystems, hash_string(fs->name), fs, NULL);
    spin_unlock_irq_restore(&registered_filesystems_lock, interrupts_enabled);
}

void vfs_unregister_fs(vfs_fs_type* fs) {
    bool interrupts_enabled = spin_lock_irq_save(&registered_filesystems_lock);
    hash_table_remove(&registered_filesystems, hash_string(fs->name));
    spin_unlock_irq_restore(&registered_filesystems_lock, interrupts_enabled);
}

u64 vfs_mount(string fs_type, string name, string path) {
    bool interrupts_enabled = spin_lock_irq_save(&registered_filesystems_lock);
    vfs_fs_type* fs =
        hash_table_get(&registered_filesystems, hash_string(fs_type));
    if (!fs) {
        spin_unlock_irq_restore(&registered_filesystems_lock,
                                interrupts_enabled);
        return -ENXIO;
    }
}

u64 vfs_walk(vfs_mount* root, vfs_dentry* pwd, string path, vfs_mode mode,
             vfs_dentry** dentry) {
    string left = path;
    vfs_mount* mount = root;
    if (left[0] == '/') {
        while (true) {
            if (root->)
        }
    }


    vfs_dentry* iter = mount->mnt_root;
    while (iter) {
        u64 path_entry_length = -1;
        left += path_entry_length;

        vfs_dentry* path_dentry = NULL;
        LINKED_LIST_FOR_EACH(&iter->children, child) {
            if (strcmp(left, ((vfs_dentry*) child->value)->name)) {
                path_dentry = child->value;
            }
        }
        if (path_dentry) {
            continue;
        } else {

        }
    }
}
