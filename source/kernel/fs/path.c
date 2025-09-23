#include "path.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/string.h"

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

    return cur_len > 0 ? count + 1 : count;
}

path_parts path_parts_from_path(string path) {
    return (path_parts) {.path = path, .parts_left = count_parts(path)};
}

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

static vfs_dentry* lookup(vfs_dentry* parent, string path) {
    if (streq(path, ".")) {
        vfs_dentry_acquire(parent);
        return parent;
    }

    if (streq(path, ".."))
        return vfs_dentry_get_parent(parent);

    vfs_dentry* child = vfs_dcache_get(parent, path);
    child = child ? child : parent->inode->ops->lookup(parent, path);
    return child;
}

u64 walk_one(vfs_path start, vfs_path* res, path_parts* parts) {
    vfs_mount* mnt = start.mount;
    vfs_dentry* dentry = start.dentry;

    if (parts->parts_left == 0)
        return -ENOENT;

    if (dentry->inode->type != DIRECTORY)
        return -ENOTDIR;

    // TODO: resolve mountpoints
    walk_next_part(parts);
    dentry = lookup(dentry, parts->part);

    *res = (vfs_path) {.dentry = dentry, .mount = mnt};
    return IS_ERROR(dentry) ? PTR_ERROR(dentry) : 0;
}

u64 walk_parent(vfs_path start, vfs_path* res, path_parts* parts) {
    vfs_path iter = start;
    vfs_dentry* dentry = iter.dentry;

    vfs_dentry_acquire(dentry);
    while (true) {
        if (parts->parts_left == 1)
            return 0;

        vfs_inode_lock_shared(dentry->inode);
        u64 error = walk_one(iter, res, parts);
        vfs_inode_unlock_shared(dentry->inode);

        vfs_dentry_release(dentry);

        if (IS_ERROR(error))
            return error;

        iter = *res;
        dentry = iter.dentry;
    }
}

u64 walk(vfs_path start, vfs_path* res, path_parts* parts) {
    u64 error = walk_parent(start, res, parts);
    if (IS_ERROR(error))
        return error;

    vfs_dentry* parent = res->dentry;
    vfs_inode_lock_shared(parent->inode);
    error = walk_one(*res, res, parts);
    vfs_inode_unlock_shared(parent->inode);

    vfs_dentry_release(parent);

    return error;
}
