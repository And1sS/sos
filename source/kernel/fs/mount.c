#include "../error/errno.h"
#include "../error/error.h"
#include "path.h"
#include "vfs.h"

static vfs_mount* root_mount = NULL;
static rw_mutex mount_mut = RW_MUTEX_STATIC_INITIALIZER;

static vfs_mount* vfs_mount_allocate(vfs_dentry* mount_root) {
    vfs_mount* mount = kmalloc(sizeof(vfs_mount));
    if (!mount)
        return ERROR_PTR(-ENOMEM);

    memset(mount, 0, sizeof(vfs_mount));

    mount->mount_root = vfs_dentry_acquire(mount_root);
    mount->mount_node = LINKED_LIST_NODE_OF(mount);
    mount->refc = 0;
    mount->lock = SPIN_LOCK_STATIC_INITIALIZER;
    mount->children = LINKED_LIST_STATIC_INITIALIZER;

    return vfs_mount_acquire(mount);
}

void vfs_mount_root(vfs_dentry* root) {
    root_mount = vfs_mount_allocate(root);
    root_mount->parent_mount = root_mount;
    root_mount->mounted_at = root;

    if (IS_ERROR(root_mount))
        panic("Can't initialize root mount");
}

vfs_mount* vfs_mount_get_root() { return root_mount; }

vfs_mount* vfs_mount_attach(vfs_mount* parent_mount, vfs_dentry* mounted_at,
                            vfs_dentry* mount_root) {

    if (!vfs_dentry_is_root(mount_root))
        return ERROR_PTR(-EPERM);

    // prevents races with unlink, rmdir, rename and other destructive
    // operations
    vfs_inode_lock(mounted_at->inode);
    if (vfs_dentry_is_orphaned(mounted_at)) {
        vfs_inode_unlock(mounted_at->inode);
        return ERROR_PTR(-ENOENT);
    }

    vfs_mount* mount = vfs_mount_allocate(mount_root);
    if (IS_ERROR(mount))
        goto out;

    mount->parent_mount = vfs_mount_acquire(parent_mount);
    mount->mounted_at = vfs_dentry_acquire(mounted_at);

    rw_mutex_lock_write(&mount_mut);

    // TODO: check that attachment won't cause cycle

    spin_lock(&parent_mount->lock);
    // insert is done into the beginning since multiple superblocks can be
    // mounted onto same dentry, latest mount should be resolved in that
    // case
    linked_list_add_first_node(&parent_mount->children, &mount->mount_node);
    spin_unlock(&parent_mount->lock);

    rw_mutex_unlock_write(&mount_mut);

    vfs_dentry_set_mountpoint(mounted_at);

out:
    vfs_inode_unlock(mounted_at->inode);

    return mount;
}

u64 vfs_mount_detach(vfs_mount* mount) {
    if (mount == root_mount)
        return -EPERM;

    u64 error = 0;
    vfs_mount* parent = NULL;
    vfs_dentry* mounted_at = NULL;

retry:
    spin_lock(&mount->lock);
    if (mount->parent_mount == mount) {
        spin_unlock(&mount->lock);
        return -EALREADY;
    }
    parent = vfs_mount_acquire(mount->parent_mount);
    mounted_at = vfs_dentry_acquire(mount->mounted_at);
    spin_unlock(&mount->lock);

    vfs_inode_lock(mounted_at->inode);
    rw_mutex_lock_write(&mount_mut);
    if (parent != mount->parent_mount) {
        rw_mutex_unlock_write(&mount_mut);
        vfs_inode_unlock(mounted_at->inode);

        vfs_mount_release(parent);
        vfs_dentry_release(mounted_at);

        goto retry;
    }

    // now we have stable references to real parent and mounted_at
    // safe to do plain reads, since writes can be done only under mount_mut
    if (mount->children.size != 0) {
        rw_mutex_unlock_write(&mount_mut);
        vfs_inode_unlock(mounted_at->inode);

        vfs_mount_release(parent);
        vfs_dentry_release(mounted_at);

        return -EBUSY;
    }

    spin_lock(&parent->lock);
    linked_list_remove_node(&parent->children, &mount->mount_node);
    spin_unlock(&parent->lock);

    spin_lock(&mount->lock);
    mount->parent_mount = mount;
    mount->mounted_at = mount->mount_root;
    spin_unlock(&mount->lock);

    rw_mutex_unlock_write(&mount_mut);
    vfs_inode_unlock(mounted_at->inode);

    vfs_super_unmount(mounted_at->inode->sb);

    vfs_mount_release(parent);
    vfs_dentry_release(mounted_at);

    return error;
}

vfs_mount* vfs_mount_acquire(vfs_mount* mount) {
    atomic_increment(&mount->refc);
    return mount;
}

void vfs_mount_release(vfs_mount* mount) {
    // fast path, transition for any refc > 1
    if (atomic_decrement_not_one(&mount->refc))
        return;

    if (mount == root_mount)
        return;

    // slow path, transition 1 -> 0, this is needed to perform parent check with
    // stable parent reference
    spin_lock(&mount->lock);
    if (atomic_decrement_and_get(&mount->refc) != 0)
        goto out;

    // mount is detached (unreachable) and this was the last reference
    if (mount->parent_mount == mount) {
        kfree(mount);
        return;
    }

    // mount is part of a tree, do nothing
out:
    spin_unlock(&mount->lock);
}

u64 vfs_mount_find(vfs_mount* mount, vfs_dentry* dentry,
                      struct vfs_path* res) {

    spin_lock(&mount->lock);
    linked_list_node* node =
        LINKED_LIST_FIND(&mount->children, subnode,
                         ((vfs_mount*) subnode->value)->mounted_at == dentry);

    vfs_mount* submount = node ? node->value : NULL;
    u64 error = submount ? 0 : -ENOENT;
    res->mount = submount ? vfs_mount_acquire(submount) : NULL;
    res->dentry = submount ? vfs_dentry_acquire(submount->mount_root) : NULL;
    spin_unlock(&mount->lock);

    return error;
}
