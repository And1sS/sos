#include "vfs.h"
#include "../error/error.h"
#include "../lib/string.h"
#include "dentry.h"

DEFINE_HASH_TABLE(vfs_registry, string, vfs_type*, strhash, streq)

static vfs_registry type_registry;

static lock type_registry_lock = SPIN_LOCK_STATIC_INITIALIZER;

void vfs_init() {
    if (!vfs_registry_init(&type_registry))
        panic("Can't init vfs type registry");

    vfs_dcache_init();
    vfs_icache_init(0);
}

vfs_type* vfs_type_create(string name) {
    string name_copy = strcpy(name);
    if (!name_copy)
        return NULL;

    vfs_type* new = (vfs_type*) kmalloc(sizeof(vfs_type));
    if (!new) {
        strfree(name_copy);
        return NULL;
    }

    memset(new, 0, sizeof(vfs_type));
    new->name = name_copy;
    new->refc = REF_COUNT_STATIC_INITIALIZER;
    return new;
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

struct vfs_dentry* vfs_mount(vfs_type* type, device* dev) {
    struct vfs_super_block* sb = vfs_super_get(type);
    if (IS_ERROR(sb))
        return ERROR_PTR(sb);

    struct vfs_dentry* dentry = type->ops->mount(sb, dev);
    vfs_super_release(sb);
    return dentry;
}

u64 split_length(string path) {
    u64 len = 0;
    for (; path[len] != '/' && path[len] != '\0'; len++)
        ;

    return len;
}

void next_split(string path, u8 buffer[256], u8* result_length) {
    u64 len = 0;
    while (*path == '/') {
        path++;
        len++;
    }

    *result_length = split_length(path);
    memcpy(buffer, (void*) path, *result_length);
    buffer[*result_length] = '\0';
    *result_length += len;
}

struct vfs_dentry* walk(struct vfs_dentry* start, string path) {
    u64 full_length = strlen(path);
    u64 walked_length = 0;

    u8 split_buffer[256];
    string path_iter = path;
    struct vfs_dentry* iter = start;

    while (iter) {
        if (walked_length == full_length)
            return iter;

        u8 length;
        next_split(path_iter, split_buffer, &length);
        string path_element = (string) split_buffer;

        walked_length += length;
        path_iter += length;

        // TODO: resolve mountpoints

        if (streq(path_element, "."))
            continue;

        struct vfs_dentry* parent;

        if (streq(path_element, "..")) {
            parent = vfs_dentry_get_parent(iter);
            vfs_dentry_release(iter);
            iter = parent;
            continue;
        }

        parent = iter;
        iter = vfs_dcache_get(parent, path_element);
        if (!iter) {
            iter = parent->inode->ops->lookup(parent, path_element);

            if (IS_ERROR(iter)) {
                vfs_dentry_release(parent);
                return iter;
            }
        }
        vfs_dentry_release(parent);
    }

    __builtin_unreachable();
}