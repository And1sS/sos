#include "vfs.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/string.h"
#include "dcache.h"
#include "inode.h"
#include "vfs_entry_registry.h"

//static vfs root_fs;
//static lock vfs_lock = SPIN_LOCK_STATIC_INITIALIZER;

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

u64 walk(struct vfs_inode* start, string path, struct vfs_inode** result) {
    u64 full_length = strlen(path);
    u64 walked_length = 0;

    u8 split_buffer[256];
    string path_iter = path;
    vfs_inode* iter = (vfs_inode*) start;

    while (iter) {
        if (walked_length == full_length) {
            *result = (struct vfs_inode*) iter;
            return 0;
        }

        u8 length;
        next_split(path_iter, split_buffer, &length);
        walked_length += length;
        path_iter += length;

        if (streq((string) split_buffer, "."))
            continue;

        if (iter->type != DIRECTORY)
            return -ENOTDIR;

        vfs_inode* parent = iter;
        iter = dcache_get(parent, (string) split_buffer);
        if (!iter) {
            u64 lookup_result = parent->ops->lookup(
                (struct vfs_inode*) parent, (string) split_buffer, (struct vfs_inode**) &iter);

            if (IS_ERROR(lookup_result))
                return lookup_result;

            dcache_put(parent, (string) split_buffer, iter);
        }
        vfs_inode_release(parent);
    }

    __builtin_unreachable();
}