#include "super_block.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/flagops.h"
#include "dcache/dentry.h"

void vfs_super_unmount(vfs_super_block* sb) {
    spin_lock(&sb->type->lock);
    if (atomic_decrement_and_get(&sb->mount_count) != 0) {
        spin_unlock(&sb->type->lock);
        vfs_super_release(sb);
        return;
    }

    SET_FLAGS(sb->flags, SUPER_DYING);
    spin_unlock(&sb->type->lock);

    dcache_evict_unused(sb);
    // TODO: clear unused inodes

    vfs_super_release(sb);
}

vfs_super_block* vfs_super_acquire(vfs_super_block* sb) {
    atomic_increment(&sb->refc);
    return sb;
}

void vfs_super_release(vfs_super_block* sb) {
    // fast path, transition for any refc > 2
    if (atomic_decrement_greater_than(&sb->refc, 2))
        return;

    // slow path, transitions 2 -> 1, 1 -> 0
    vfs_type* type = sb->type;
    spin_lock(&type->lock);
    u64 refc = atomic_decrement_and_get(&sb->refc);
    if (refc == 0) {
        spin_unlock(&type->lock);
        vfs_super_destroy(sb);
        return;
    }

    // Safe to do plain read since at this point root is stabilized to be
    // either NULL or real root which won't change
    vfs_dentry* root = sb->root;
    if (!root || refc > 1) {
        spin_unlock(&type->lock);
        return;
    }

    // Here refc == 1 and root exists - only root dentry(and its inode) alive
    if (TEST_FLAG(sb->flags, SUPER_DYING)) {
        linked_list_remove_node(&type->super_blocks, &sb->self_node);
        spin_unlock(&type->lock);
        vfs_dentry_release(root);
        return;
    } else if (TEST_FLAG(sb->flags, SUPER_INITIALIZATION_FAILED)) {
        spin_unlock(&type->lock);
        vfs_dentry_release(root);
        return;
    }
    spin_unlock(&type->lock);

    // Root exists, refcount == 1 (root holds reference to sb), initialized
    // successfully and not dying - do nothing, just exit
}

void vfs_super_destroy(vfs_super_block* sb) {
    vfs_type_release(sb->type);
    // TODO: release device reference

    kfree(sb);
}

static vfs_super_block* vfs_super_allocate(vfs_type* type, device* dev) {
    vfs_super_block* sb = kmalloc(sizeof(vfs_super_block));
    if (!sb)
        return ERROR_PTR(-ENOMEM);

    memset(sb, 0, sizeof(vfs_super_block));

    sb->id = 0; // TODO: add id generation
    sb->type = vfs_type_acquire(type);
    sb->device = dev; // TODO: add reference counting

    sb->flags = 0;
    sb->mount_count = 1; // allocation implies that obtained sb will be mounted
    sb->refc = 0;

    sb->rename_mut = MUTEX_STATIC_INITIALIZER;

    sb->self_node = LINKED_LIST_NODE_OF(sb);

    sb->lock = SPIN_LOCK_STATIC_INITIALIZER;
    sb->initialization_cvar = CON_VAR_STATIC_INITIALIZER;

    linked_list_add_last_node(&type->super_blocks, &sb->self_node);

    return vfs_super_acquire(sb);
}

void vfs_super_unlock_new(vfs_super_block* sb) {
    spin_lock(&sb->lock);
    sb->flags |= SUPER_INITIALIZED;
    con_var_broadcast(&sb->initialization_cvar);
    spin_unlock(&sb->lock);
}

void vfs_super_unlock_failed(vfs_super_block* sb) {
    vfs_type* type = sb->type;
    spin_lock(&type->lock);
    linked_list_remove_node(&type->super_blocks, &sb->self_node);
    spin_unlock(&type->lock);

    spin_lock(&sb->lock);
    sb->flags |= SUPER_INITIALIZATION_FAILED;
    con_var_broadcast(&sb->initialization_cvar);
    spin_unlock(&sb->lock);
}

static bool vfs_super_initialization_finished(vfs_super_block* sb) {
    return (sb->flags & SUPER_INITIALIZED)
           || (sb->flags & SUPER_INITIALIZATION_FAILED);
}

static bool vfs_super_await_initialization(vfs_super_block* sb) {
    if (TEST_FLAG(sb->flags, SUPER_INITIALIZED))
        return true;

    spin_lock(&sb->lock);
    CON_VAR_WAIT_FOR(&sb->initialization_cvar, &sb->lock,
                     vfs_super_initialization_finished(sb));
    spin_unlock(&sb->lock);

    return TEST_FLAG(sb->flags, SUPER_INITIALIZED);
}

static vfs_super_block* vfs_super_find(vfs_type* type, device* dev) {
    // TODO: change equality to test fs callback
    linked_list_node* node =
        LINKED_LIST_FIND(&type->super_blocks, subnode,
                         ((vfs_super_block*) subnode->value)->device == dev);

    return node ? node->value : NULL;
}

vfs_super_block* vfs_super_get(vfs_type* type, device* dev) {
retry:
    spin_lock(&type->lock);
    vfs_super_block* sb = vfs_super_find(type, dev);
    if (!sb) {
        sb = vfs_super_allocate(type, dev);
        goto out;
    }

    if (TEST_FLAG(sb->flags, SUPER_DYING)) {
        sb = ERROR_PTR(-EBUSY);
        goto out;
    }

    // Increment mount count in case superblock is/will be initialised.
    // In initialisation fails - increased mount_count won't do
    // anything anyway.
    atomic_increment(&sb->mount_count);
    vfs_super_acquire(sb);
    spin_unlock(&type->lock);

    if (!vfs_super_await_initialization(sb)) {
        vfs_super_release(sb);
        goto retry;
    }

    return sb;

out:
    spin_unlock(&type->lock);

    return sb;
}

bool vfs_super_is_dying(vfs_super_block* sb) {
    return TEST_FLAG(sb->flags, SUPER_DYING);
}

bool vfs_super_is_initialization_failed(vfs_super_block* sb) {
    return TEST_FLAG(sb->flags, SUPER_INITIALIZATION_FAILED);
}
