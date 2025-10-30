#include "vfs.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/string.h"
#include "dcache/dcache.h"
#include "dcache/dentry.h"
#include "mount.h"
#include "path.h"
#include "ramfs/ramfs.h"

DEFINE_HASH_TABLE(vfs_registry, string, struct vfs_type*, strhash, streq)

static lock type_registry_lock = SPIN_LOCK_STATIC_INITIALIZER;

static vfs_registry type_registry;

void vfs_init() {
    if (!vfs_registry_init(&type_registry))
        panic("Can't init vfs type registry");

    // TODO: calculate based on available ram or get from config
    dcache_init(4, 10000);
    vfs_icache_init(10000);

    ramfs_init();
    vfs_type* rootfs_type = vfs_registry_get(&type_registry, RAMFS_NAME);
    vfs_dentry* rootfs_root = rootfs_type->ops->mount(rootfs_type, NULL);
    if (IS_ERROR(rootfs_root))
        panic("Can't mount root filesystem");

    vfs_mount_root(rootfs_root);
    vfs_dentry_release(rootfs_root);
}

vfs_type* vfs_type_create(string name) {
    string name_copy = strcpy(name);
    if (!name_copy)
        return ERROR_PTR(-ENOMEM);

    vfs_type* new = kmalloc(sizeof(vfs_type));
    if (!new) {
        strfree(name_copy);
        return ERROR_PTR(-ENOMEM);
    }

    memset(new, 0, sizeof(vfs_type));
    new->name = name_copy;
    new->refc = REF_COUNT_STATIC_INITIALIZER;
    return new;
}

void vfs_type_destroy(vfs_type* type) {
    strfree(type->name);
    kfree(type);
}

bool register_vfs_type(vfs_type* type) {
    type->lock = SPIN_LOCK_STATIC_INITIALIZER;
    type->super_blocks = LINKED_LIST_STATIC_INITIALIZER;
    type->refc = REF_COUNT_STATIC_INITIALIZER;

    bool interrupts_enabled = spin_lock_irq_save(&type_registry_lock);

    bool registered =
        !vfs_registry_get(&type_registry, type->name)
        && vfs_registry_put(&type_registry, type->name, type, NULL);

    spin_unlock_irq_restore(&type_registry_lock, interrupts_enabled);

    return registered;
}

void deregister_vfs_type(vfs_type* type) {
    bool interrupts_enabled = spin_lock_irq_save(&type_registry_lock);
    vfs_registry_remove(&type_registry, type->name);
    spin_unlock_irq_restore(&type_registry_lock, interrupts_enabled);
}

u64 vfs_unlink(vfs_path start, string path) {
    if (path_ends_with_dot(path) || path_ends_with_dotdot(path))
        return -EPERM;

    vfs_path res;
    path_parts parts = path_parts_from_path(path);

    u64 error = walk_parent(start, &res, &parts);
    if (IS_ERROR(error))
        return error;

    vfs_dentry* parent = res.dentry;
    vfs_inode* dir = parent->inode;

    vfs_inode_lock(dir);
    error = walk_one(res, &res, &parts);
    if (IS_ERROR(error))
        goto out;

    vfs_dentry* child = res.dentry;
    error = -EISDIR;
    if (child->inode->type == DIRECTORY)
        goto failed_to_unlink;

    error = -EPERM;
    if (!dir->ops->unlink)
        goto failed_to_unlink;

    error = dir->ops->unlink(dir, child);
    if (!IS_ERROR(error))
        vfs_dentry_unlink(child);

failed_to_unlink:
    vfs_dentry_release(child);

out:
    vfs_inode_unlock(dir);

    vfs_dentry_release(parent);
    return error;
}

static void vfs_rename_lock(vfs_dentry* old_parent, vfs_dentry* new_parent) {
    mutex_lock(&old_parent->inode->sb->rename_mut);

    if (old_parent == new_parent)
        vfs_inode_lock(old_parent->inode);
    else if (vfs_dentry_is_ancestor(old_parent, new_parent)) {
        vfs_inode_lock(new_parent->inode);
        vfs_inode_lock(old_parent->inode);
    } else if (vfs_dentry_is_ancestor(new_parent, old_parent)) {
        vfs_inode_lock(old_parent->inode);
        vfs_inode_lock(new_parent->inode);
    } else
        vfs_inodes_lock(old_parent->inode, new_parent->inode);
}

static void vfs_rename_unlock(vfs_dentry* old_parent, vfs_dentry* new_parent) {
    if (old_parent == new_parent)
        vfs_inode_unlock(old_parent->inode);
    else // unlock order doesn't matter
        vfs_inodes_unlock(old_parent->inode, new_parent->inode);

    mutex_unlock(&old_parent->inode->sb->rename_mut);
}

u64 vfs_rename(vfs_path old_parent, vfs_dentry* old_dentry, vfs_path new_parent,
               string new_name) {

    string new_name_copy = strcpy(new_name);
    if (!new_name_copy)
        return -ENOMEM;

    vfs_dentry* old_parent_dentry = old_parent.dentry;
    vfs_dentry* new_parent_dentry = new_parent.dentry;

    // check that parents are in same superblock, and we are renaming (moving)
    // into a directory
    if (old_parent_dentry->inode->sb != new_parent_dentry->inode->sb
        || !vfs_dentry_is_dir(new_parent_dentry)) {
        strfree(new_name_copy);
        return -EPERM;
    }

    vfs_rename_lock(old_parent_dentry, new_parent_dentry);

    // recheck that parents are still alive
    u64 error = -ENOENT;
    if (vfs_dentry_is_orphaned(new_parent_dentry))
        goto out;

    error = -EPERM;
    if (!old_dentry->inode->ops->rename)
        goto out;

    // recheck that old_dentry hasn't been moved while we weren't holding lock
    // safe to read parent since we are holding old_parent_dentry->inode->rw_mut
    // which carries visibility and prevents concurrent modifications
    error = -ENOENT;
    if (old_dentry->parent != old_parent_dentry)
        goto out;

    // check that we are not moving directory into its subdirectory
    error = -EINVAL;
    if (vfs_dentry_is_ancestor(new_parent_dentry, old_dentry))
        goto out;

    // lookup victim that will be replaced
    vfs_dentry* victim_dentry = lookup(new_parent_dentry, new_name);
    error = IS_ERROR(victim_dentry) ? PTR_ERROR(victim_dentry) : 0;
    victim_dentry = IS_ERROR(victim_dentry) ? NULL : victim_dentry;
    if (error && error != (u64) -ENOENT)
        goto out;

    // TODO: add check for mountpoints

    error = old_dentry->inode->ops->rename(old_parent_dentry, old_dentry,
                                           new_parent_dentry, victim_dentry,
                                           new_name_copy);

    if (!error)
        vfs_dentry_move(new_parent_dentry, old_dentry, new_name_copy);

    if (victim_dentry)
        vfs_dentry_release(victim_dentry);

out:
    if (error)
        strfree(new_name_copy);

    vfs_rename_unlock(old_parent_dentry, new_parent_dentry);

    return error;
}