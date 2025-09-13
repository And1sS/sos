#include "vfs.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/string.h"
#include "dentry.h"
#include "mount.h"
#include "ramfs/ramfs.h"

DEFINE_HASH_TABLE(vfs_registry, string, struct vfs_type*, strhash, streq)

static vfs_registry type_registry;

static lock type_registry_lock = SPIN_LOCK_STATIC_INITIALIZER;

void vfs_init() {
    if (!vfs_registry_init(&type_registry))
        panic("Can't init vfs type registry");

    vfs_dcache_init();
    vfs_icache_init(0);

    ramfs_init();
    struct vfs_type* rootfs_type = vfs_registry_get(&type_registry, RAMFS_NAME);
    vfs_mount_root(rootfs_type->ops->mount(rootfs_type, NULL));
}

struct vfs_type* vfs_type_create(string name) {
    string name_copy = strcpy(name);
    if (!name_copy)
        return ERROR_PTR(-ENOMEM);

    struct vfs_type* new = (struct vfs_type*) kmalloc(sizeof(struct vfs_type));
    if (!new) {
        strfree(name_copy);
        return ERROR_PTR(-ENOMEM);
    }

    memset(new, 0, sizeof(struct vfs_type));
    new->name = name_copy;
    new->refc = REF_COUNT_STATIC_INITIALIZER;
    return new;
}

void vfs_type_destroy(struct vfs_type* type) {
    strfree(type->name);
    kfree(type);
}

bool register_vfs_type(struct vfs_type* type) {
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

void deregister_vfs_type(struct vfs_type* type) {
    bool interrupts_enabled = spin_lock_irq_save(&type_registry_lock);
    vfs_registry_remove(&type_registry, type->name);
    spin_unlock_irq_restore(&type_registry_lock, interrupts_enabled);
}