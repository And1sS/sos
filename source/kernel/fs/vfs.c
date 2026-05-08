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
    vfs_mount_tree_init();

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

vfs_type* vfs_type_get(string name) {
    return vfs_registry_get(&type_registry, name);
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

vfs_type* vfs_type_acquire(vfs_type* type) {
    ref_acquire(&type->refc);
    return type;
}

void vfs_type_release(vfs_type* type) {
    spin_lock(&type->lock);
    ref_release(&type->refc);
    spin_unlock(&type->lock);
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
    vfs_inode_lock(child->inode);

    error = -EISDIR;
    if (child->inode->type == DIRECTORY)
        goto failed_to_unlink;

    error = -EBUSY;
    // can only report invalid state when we raced with unmouning and missed
    // flag change to false, but it is harmless in this case
    if (vfs_dentry_is_mountpoint(child))
        goto failed_to_unlink;

    error = -EPERM;
    if (!dir->ops->unlink)
        goto failed_to_unlink;

    error = dir->ops->unlink(dir, child);
    if (!IS_ERROR(error))
        vfs_dentry_unlink(child);

failed_to_unlink:
    vfs_inode_unlock(child->inode);
    vfs_dentry_release(child);
out:
    vfs_inode_unlock(dir);
    vfs_dentry_release(parent);

    return error;
}

static void vfs_rename_lock(vfs_dentry* old_dir, vfs_dentry* new_dir) {
    if (old_dir == new_dir) {
        vfs_inode_lock(old_dir->inode);
        return;
    }

    mutex_lock(&old_dir->inode->sb->rename_mut);
    if (vfs_dentry_is_ancestor(old_dir, new_dir)) {
        vfs_inode_lock(new_dir->inode);
        vfs_inode_lock(old_dir->inode);
    } else if (vfs_dentry_is_ancestor(new_dir, old_dir)) {
        vfs_inode_lock(old_dir->inode);
        vfs_inode_lock(new_dir->inode);
    } else
        vfs_inodes_lock(old_dir->inode, new_dir->inode);
}

static void vfs_rename_unlock(vfs_dentry* old_dir, vfs_dentry* new_dir) {
    if (old_dir == new_dir)
        vfs_inode_unlock(old_dir->inode);
    else {
        // unlock order doesn't matter
        vfs_inodes_unlock(old_dir->inode, new_dir->inode);
        mutex_unlock(&old_dir->inode->sb->rename_mut);
    }
}

static void vfs_rename_lock_children(vfs_dentry* source, vfs_dentry* target) {
    if (!target)
        vfs_inode_lock(source->inode);
    else
        vfs_inodes_lock(source->inode, target->inode);
}

static void vfs_rename_unlock_children(vfs_dentry* source, vfs_dentry* target) {
    if (!target)
        vfs_inode_unlock(source->inode);
    else
        vfs_inodes_unlock(source->inode, target->inode);
}

u64 vfs_rename(vfs_path old_dir, vfs_dentry* source, vfs_path new_dir,
               string name) {

    vfs_dentry* old_dir_dentry = old_dir.dentry;
    vfs_dentry* new_dir_dentry = new_dir.dentry;

    if (path_ends_with_dot(name) || path_ends_with_dotdot(name))
        return -EPERM;

    string name_copy = strcpy(name);
    if (!name_copy)
        return -ENOMEM;

    u64 error = -EPERM;
    // check that new_dir is directory and rename is supported
    if (!vfs_dentry_is_dir(new_dir_dentry) || !source->inode->ops->rename)
        goto out;

    error = -EXDEV;
    if (old_dir.mount != new_dir.mount) // check that parents are in same mnt
        goto out;

    vfs_rename_lock(old_dir_dentry, new_dir_dentry);

    // lookup victim that will be replaced, no need for crossing mount since
    // victim is not allowed to be mountpoint anyway
    vfs_dentry* target = lookup_child(new_dir_dentry, name);
    error = IS_ERROR(target) ? PTR_ERROR(target) : 0;
    target = target && !IS_ERROR(target) ? target : NULL;
    if (error && error != (u64) -ENOENT)
        goto out_parents_locked;

    // check that we are not changing anything in case old name == new name
    error = -EEXIST;
    if (target == source)
        goto out_parents_locked;

    // prevent concurrent changes to source or target topologies
    vfs_rename_lock_children(source, target);

    // recheck that new dir and source are still alive or
    // source hasn't been moved while we weren't holding lock
    error = -ENOENT;
    if (vfs_dentry_is_orphaned(new_dir_dentry) || vfs_dentry_is_orphaned(source)
        || source->parent != old_dir_dentry)
        goto out_children_locked;

    // check that we are not moving directory into its subdirectory
    error = -EINVAL;
    if (vfs_dentry_is_ancestor(new_dir_dentry, source))
        goto out_children_locked;

    // prevent concurrent mnt tree changes, needed to prevent
    // mounting/unmounting down the subtrees involved
    vfs_mount_tree_lock_shared();

    error = -EBUSY;
    if (has_submounts(source) || (target && has_submounts(target)))
        goto error_dentry;

    error = source->inode->ops->rename(old_dir_dentry, source, new_dir_dentry,
                                       target, name_copy);

    if (!error)
        vfs_dentry_move(new_dir_dentry, source, name_copy);

error_dentry:
    vfs_mount_tree_unlock_shared();

out_children_locked:
    vfs_rename_unlock_children(source, target);

out_parents_locked:
    if (target)
        vfs_dentry_release(target);

    vfs_rename_unlock(old_dir_dentry, new_dir_dentry);
out:
    if (error)
        strfree(name_copy);

    return error;
}