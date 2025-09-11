#include "scache.h"

void vfs_super_acquire(struct vfs_super_block* sb) { ref_acquire(&sb->refc); }
void vfs_super_release(struct vfs_super_block* sb) {
    UNUSED(sb);
}

struct vfs_super_block* vfs_super_get(vfs_type* type) {
    UNUSED(type);

    struct vfs_super_block* sb = kmalloc(sizeof(struct vfs_super_block));
    memset(sb, 0, sizeof(struct vfs_super_block));

    sb->id = 0;
    sb->refc = REF_COUNT_STATIC_INITIALIZER;
    sb->lock = SPIN_LOCK_STATIC_INITIALIZER;
    sb->type = type;

    return sb;
}
