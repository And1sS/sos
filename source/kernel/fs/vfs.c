#include "vfs.h"
#include "../error/error.h"
#include "../lib/string.h"
#include "dcache.h"

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
        walked_length += length;
        path_iter += length;

        string path_element = (string) split_buffer;
        if (streq((string) split_buffer, "."))
            continue;

        struct vfs_dentry* parent = iter;
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