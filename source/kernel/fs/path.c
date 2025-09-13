#include "path.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/string.h"

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

void vfs_path_release(vfs_path* path) {
    vfs_dentry_release(path->dentry);
    vfs_mount_release(path->mount);
}

u64 walk(vfs_path start, string path, vfs_path* res) {
    u64 full_length = strlen(path);
    u64 walked_length = 0;

    u8 split_buffer[256];
    string path_iter = path;

    vfs_mount* iter_mnt = start.mount;
    struct vfs_dentry* iter = start.dentry;

    while (iter) {
        if (walked_length == full_length) {
            res->mount = iter_mnt;
            res->dentry = iter;
            return 0;
        }

        if (iter->inode->type != DIRECTORY) {
            vfs_dentry_release(iter);
            return -ENOTDIR;
        }

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
                return PTR_ERROR(iter);
            }
        }
        vfs_dentry_release(parent);
    }

    __builtin_unreachable();
}
