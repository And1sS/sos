#include "mount.h"
#include "../error/errno.h"
#include "../error/error.h"

static vfs_mount* root_mount = NULL;

static vfs_mount* vfs_mount_allocate(vfs_dentry* mount_root) {
    vfs_mount* mount = kmalloc(sizeof(vfs_mount));
    if (!mount)
        return ERROR_PTR(-ENOMEM);

    memset(mount, 0, sizeof(vfs_mount));

    mount->mount_root = vfs_dentry_acquire(mount_root);

    mount->mut = RW_MUTEX_STATIC_INITIALIZER;

    mount->lock = SPIN_LOCK_STATIC_INITIALIZER;
    mount->refc = REF_COUNT_STATIC_INITIALIZER;

    return vfs_mount_acquire(mount);
}

void vfs_mount_root(vfs_dentry* root) {
    root_mount = vfs_mount_allocate(root);
    if (IS_ERROR(root_mount))
        panic("Can't initialize root mount");
}

// caller should hold parent_mount mut for write
vfs_mount* vfs_mount_create(vfs_mount* parent_mount, vfs_dentry* mounted_at,
                            vfs_dentry* mount_root) {

    vfs_mount* mount = vfs_mount_allocate(mount_root);
    if (IS_ERROR(mount))
        return mount;

    mount->parent_mount = vfs_mount_acquire(parent_mount);
    mount->mounted_at = vfs_dentry_acquire(mounted_at);

    mount->mount_node = LINKED_LIST_NODE_OF(mount);

    mount->lock = SPIN_LOCK_STATIC_INITIALIZER;
    mount->children = LINKED_LIST_STATIC_INITIALIZER;
    mount->refc = REF_COUNT_STATIC_INITIALIZER;

    // insert is done into the beginning since multiple superblocks can be
    // mounted onto same dentry, latest mount should be resolved in that
    // case
    spin_lock(&parent_mount->lock);
    linked_list_add_first_node(&parent_mount->children, &mount->mount_node);
    spin_unlock(&parent_mount->lock);

    vfs_dentry_set_mountpoint(mounted_at);

    return vfs_mount_acquire(mount);
}

vfs_mount* vfs_mount_get_root() { return root_mount; }

vfs_mount* vfs_mount_acquire(vfs_mount* mount) {
    ref_acquire(&mount->refc);
    return mount;
}

void vfs_mount_release(vfs_mount* mount) {
    spin_lock(&mount->lock);
    ref_release(&mount->refc);
    spin_unlock(&mount->lock);
}

vfs_dentry* vfs_mount_resolve(vfs_mount* mount, vfs_dentry* dentry) {
    spin_lock(&mount->lock);
    linked_list_node* node =
        LINKED_LIST_FIND(&mount->children, subnode,
                         ((vfs_mount*) subnode->value)->mounted_at == dentry);

    vfs_dentry* mount_root = node ? vfs_dentry_acquire(node->value) : NULL;
    spin_unlock(&mount->lock);

    return mount_root;
}
