#include "mount.h"

static vfs_mount* root_mount = NULL;
static lock mounts_lock = SPIN_LOCK_STATIC_INITIALIZER;

void vfs_mount_root(struct vfs_dentry* root) {
    spin_lock(&mounts_lock);
    if (root_mount)
        panic("Trying to remount root fs");

    root_mount = kmalloc(sizeof(struct vfs_mount));
    if (!root_mount)
        panic("Can't initialize root mount");

    memset(root_mount, 0, sizeof(vfs_mount));
    root_mount->mount_root = root;
    spin_unlock(&mounts_lock);

    vfs_dentry_acquire(root);
}

vfs_mount* vfs_mount_get_root() { return root_mount; }

void vfs_mount_acquire(vfs_mount* mount) { ref_acquire(&mount->refc); }

void vfs_mount_release(vfs_mount* mount) {
    spin_lock(&mount->lock);
    ref_release(&mount->refc);
    spin_unlock(&mount->lock);
}
