#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/hash.h"
#include "path.h"
#include "vfs.h"

static u64 key_hash(vfs_super_block* key) { return hash((u64) key); }
static bool key_equals(vfs_super_block* a, vfs_super_block* b) {
    return a == b;
}
DEFINE_HASH_TABLE(mnt_registry, vfs_super_block*, array_list*, key_hash,
                  key_equals)

static vfs_mount* root_mount = NULL;
// protects both mount tree and mount registry from changes
static rw_mutex mount_tree_mut = RW_MUTEX_STATIC_INITIALIZER;

// hosting super_block to all hosted mounts mappings
static mnt_registry mount_registry;

// these should be accessed under mount_tree_mut locked for write
static bool mount_registry_add(vfs_mount* mount) {
    vfs_super_block* sb = mount->mounted_at->inode->sb;
    array_list* mounts = mnt_registry_get(&mount_registry, sb);
    if (!mounts) {
        mounts = array_list_create(8);
        if (!mounts)
            return false;

        if (!mnt_registry_put(&mount_registry, sb, mounts, NULL)) {
            array_list_destroy(mounts);
            return false;
        }
    }

    bool added = array_list_add_last(mounts, mount);
    if (!mounts->size) {
        mnt_registry_remove(&mount_registry, sb);
        array_list_destroy(mounts);
    }

    return added;
}

static void mount_registry_remove(vfs_mount* mount) {
    vfs_super_block* sb = mount->mounted_at->inode->sb;
    array_list* mounts = mnt_registry_get(&mount_registry, sb);
    if (!mounts)
        return;

    array_list_remove(mounts, mount);
    if (!mounts->size) {
        mnt_registry_remove(&mount_registry, sb);
        array_list_destroy(mounts);
    }
}

void vfs_mount_tree_init() {
    if (!mnt_registry_init(&mount_registry))
        panic("Can't init mount tree");
}

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

    if (IS_ERROR(root_mount) || !mount_registry_add(root_mount))
        panic("Can't initialize root mount");
}

vfs_mount* vfs_mount_get_root() { return root_mount; }

vfs_mount* vfs_mount_attach(vfs_mount* parent_mount, vfs_dentry* mounted_at,
                            vfs_dentry* mount_root) {

    // allow to mount only superblock roots
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

    vfs_mount_tree_lock();
    // TODO: check that parent mount is not detached
    // TODO: check that attachment won't cause cycle

    if (!mount_registry_add(mount)) {
        // TODO: cleanup broken mount
    }

    spin_lock(&parent_mount->lock);
    // insert is done into the beginning since multiple superblocks can be
    // mounted onto same dentry, latest mount should be resolved in that
    // case
    linked_list_add_first_node(&parent_mount->children, &mount->mount_node);
    spin_unlock(&parent_mount->lock);

    vfs_dentry_set_mountpoint(mounted_at);
    vfs_mount_tree_unlock();

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

    vfs_mount_tree_lock();

    if (parent != mount->parent_mount) {
        vfs_mount_tree_unlock();
        vfs_mount_release(parent);
        vfs_dentry_release(mounted_at);

        goto retry;
    }

    // now we have stable references to real parent and mounted_at
    // safe to do plain reads, since writes can be done only under
    // mount_tree_mut
    if (mount->children.size != 0) {
        vfs_mount_tree_unlock();
        vfs_mount_release(parent);
        vfs_dentry_release(mounted_at);

        return -EBUSY;
    }

    mount_registry_remove(mount);
    // TODO: set dentry->flags | DENTRY_MOUNTPOINT to 0

    spin_lock(&parent->lock);
    linked_list_remove_node(&parent->children, &mount->mount_node);
    spin_unlock(&parent->lock);

    spin_lock(&mount->lock);
    mount->parent_mount = mount;
    mount->mounted_at = mount->mount_root;
    spin_unlock(&mount->lock);

    vfs_mount_tree_unlock();
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

u64 vfs_mount_walk_down(vfs_mount* mount, vfs_dentry* dentry,
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

u64 vfs_mount_walk_up(vfs_mount* mount, vfs_path* res) {
    spin_lock(&mount->lock);
    res->dentry = vfs_dentry_acquire(mount->mounted_at);
    res->mount = vfs_mount_acquire(mount->parent_mount);
    spin_unlock(&mount->lock);

    return 0;
}

bool has_submounts(vfs_dentry* root) {
    array_list* mounts = mnt_registry_get(&mount_registry, root->inode->sb);
    if (!mounts)
        return false;

    ARRAY_LIST_FOR_EACH(mounts, vfs_mount * iter) {
        if (iter->mounted_at == root
            || vfs_dentry_is_ancestor(iter->mounted_at, root))
            return true;
    }
    return false;
}

void vfs_mount_tree_lock_shared() { rw_mutex_lock_read(&mount_tree_mut); }

void vfs_mount_tree_unlock_shared() { rw_mutex_unlock_read(&mount_tree_mut); }

void vfs_mount_tree_lock() { rw_mutex_lock_write(&mount_tree_mut); }

void vfs_mount_tree_unlock() { rw_mutex_unlock_write(&mount_tree_mut); }