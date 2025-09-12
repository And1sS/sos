#include "super_block.h"
#include "../error/errno.h"
#include "../error/error.h"

// Lock order -> type -> superblock;

void vfs_super_acquire(struct vfs_super_block* sb) { ref_acquire(&sb->refc); }

void vfs_super_release(struct vfs_super_block* sb) {
    spin_lock(&sb->lock);
    ref_release(&sb->refc);
    spin_unlock(&sb->lock);
}

u64 vfs_super_destroy(struct vfs_super_block* sb) {
    vfs_type* type = sb->type;

    spin_lock(&type->lock);
    spin_lock(&sb->lock);
    if (sb->refc.count != 0) {
        spin_unlock(&sb->lock);
        spin_unlock(&type->lock);
        return -EBUSY;
    }

    bool free = !sb->dying;
    if (!free) {
        spin_unlock(&sb->lock);
        spin_unlock(&type->lock);
        return -EALREADY;
    }

    sb->dying = true;

retry:
    spin_unlock(&type->lock);
    CON_VAR_WAIT_FOR(&sb->refc.empty_cvar, &sb->lock, sb->refc.count == 0);
    spin_unlock(&sb->lock);

    spin_lock(&type->lock);
    spin_lock(&sb->lock);
    if (sb->refc.count != 0) // someone acquired reference through type while we
        goto retry;          // were not holding type lock

    linked_list_remove_node(&type->super_blocks, &sb->self_node);
    ref_release(&type->refc);
    spin_unlock(&type->lock);

    kfree(sb);
    return 0;
}

static struct vfs_super_block* vfs_super_allocate(vfs_type* type) {
    struct vfs_super_block* sb = kmalloc(sizeof(struct vfs_super_block));
    if (!sb)
        return ERROR_PTR(-ENOMEM);

    memset(sb, 0, sizeof(struct vfs_super_block));

    sb->id = 0; // TODO: add id generation
    sb->type = type;

    sb->self_node = LINKED_LIST_NODE_OF(sb);
    sb->lock = SPIN_LOCK_STATIC_INITIALIZER;
    sb->dying = false;
    sb->refc = REF_COUNT_STATIC_INITIALIZER;

    return sb;
}

struct vfs_super_block* vfs_super_get(vfs_type* type) {
    struct vfs_super_block* sb = vfs_super_allocate(type);
    if (IS_ERROR(sb))
        return sb;

    ref_acquire(&sb->refc);

    spin_lock(&type->lock);
    ref_acquire(&type->refc);
    linked_list_add_last_node(&type->super_blocks, &sb->self_node);
    spin_unlock(&type->lock);

    return sb;
}
