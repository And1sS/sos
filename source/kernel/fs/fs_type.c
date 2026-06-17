#include "fs_type.h"

#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/container/hash_table/hash_table.h"
#include "../lib/panic.h"

DEFINE_HASH_TABLE(vfs_registry, string, struct vfs_type*, strhash, streq)

static lock type_registry_lock = SPIN_LOCK_STATIC_INITIALIZER;
static vfs_registry type_registry;

void vfs_type_registry_init() {
    if (!vfs_registry_init(&type_registry))
        panic("Can't init vfs type registry");
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
    new->refc = 0;
    return new;
}

vfs_type* vfs_type_get(string name) {
    bool interrupts_enabled = spin_lock_irq_save(&type_registry_lock);
    vfs_type* type = vfs_registry_get(&type_registry, name);
    if (type)
        vfs_type_acquire(type);
    spin_unlock_irq_restore(&type_registry_lock, interrupts_enabled);

    return type;
}

void vfs_type_destroy(vfs_type* type) {
    strfree(type->name);
    kfree(type);
}

bool vfs_type_register(vfs_type* type) {
    type->lock = SPIN_LOCK_STATIC_INITIALIZER;
    type->super_blocks = LINKED_LIST_STATIC_INITIALIZER;
    type->refc = 0;

    bool interrupts_enabled = spin_lock_irq_save(&type_registry_lock);

    bool registered =
        !vfs_registry_get(&type_registry, type->name)
        && vfs_registry_put(&type_registry, type->name, type, NULL);

    spin_unlock_irq_restore(&type_registry_lock, interrupts_enabled);

    return registered;
}

void vfs_type_deregister(vfs_type* type) {
    bool interrupts_enabled = spin_lock_irq_save(&type_registry_lock);
    vfs_registry_remove(&type_registry, type->name);
    spin_unlock_irq_restore(&type_registry_lock, interrupts_enabled);
}

vfs_type* vfs_type_acquire(vfs_type* type) {
    atomic_increment(&type->refc);
    return type;
}

void vfs_type_release(vfs_type* type) {
    if (atomic_decrement_and_get(&type->refc) == 0)
        vfs_type_destroy(type);
}
