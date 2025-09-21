#include "path.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/string.h"

#define MAX_PATH_LENGTH 512

typedef struct {
    char part[MAX_PATH_LENGTH];
    u64 parts_left;
    string path;
} path_parts;

static u64 count_parts(string path) {
    u64 count = 0;
    u64 cur_len = 0;
    while (*path != '\0') {
        if (*path == '/' && cur_len > 0) {
            cur_len = 0;
            count++;
        } else if (*path != '/') {
            cur_len++;
        }
        path++;
    }

    return count;
}

static bool parts_left(path_parts* parts) { return parts->parts_left > 0; }

static u64 part_length(string path) {
    u64 len = 0;
    for (; path[len] != '/' && path[len] != '\0' && len < MAX_PATH_LENGTH - 1;
         len++)
        ;

    return len;
}

static void walk_next_part(path_parts* parts) {
    while (*parts->path == '/')
        parts->path++;

    u64 part_len = part_length(parts->path);
    memcpy(parts->part, (void*) parts->path, part_len);
    parts->part[part_len] = '\0';
    parts->path = parts->path + part_len;
    parts->parts_left--;
}

static struct vfs_dentry* lookup(struct vfs_dentry* parent, string path) {
    if (streq(path, ".")) {
        vfs_dentry_acquire(parent);
        return parent;
    }

    if (streq(path, ".."))
        return vfs_dentry_get_parent(parent);

    vfs_inode_lock_shared(parent->inode);
    struct vfs_dentry* child = vfs_dcache_get(parent, path);
    child = child ? child : parent->inode->ops->lookup(parent, path);
    vfs_inode_unlock_shared(parent->inode);
    return child;
}

u64 walk(vfs_path start, string path, vfs_path* res) {
    path_parts parts = {.path = path, .parts_left = count_parts(path)};
    vfs_mount* mnt = start.mount;
    struct vfs_dentry* iter = start.dentry;
    // upstream already holds the reference, but we need different one for walk
    vfs_dentry_acquire(iter);

    while (iter) {
        if (!parts_left(&parts)) {
            *res = (vfs_path) {.mount = mnt, .dentry = iter};
            return 0;
        }

        if (iter->inode->type != DIRECTORY) {
            vfs_dentry_release(iter);
            return -ENOTDIR;
        }

        // TODO: resolve mountpoints
        struct vfs_dentry* parent = iter;
        walk_next_part(&parts);
        iter = lookup(parent, parts.part);
        vfs_dentry_release(parent);

        if (IS_ERROR(iter))
            return PTR_ERROR(iter);
    }

    __builtin_unreachable();
}
