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

bool path_ends_with_dot(string path) {
    u64 len = strlen(path);
    if (len < 1)
        return false;

    if (streq(path, "."))
        return true;

    return len >= 2 && path[len - 2] == '/' && path[len - 1] == '.';
}

bool path_ends_with_dotdot(string path) {
    u64 len = strlen(path);
    if (len < 2)
        return false;

    if (streq(path, ".."))
        return true;

    return len >= 3 && path[len - 3] == '/' && path[len - 2] == '.'
           && path[len - 1] == '.';
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

static string walk_next_part(path_parts* parts) {
    while (*parts->path == '/')
        parts->path++;

    u64 part_len = part_length(parts->path);
    memcpy(parts->part, (void*) parts->path, part_len);
    parts->part[part_len] = '\0';
    parts->path = parts->path + part_len;
    parts->parts_left--;

    return parts->part;
}

vfs_dentry* lookup(vfs_dentry* dir, string path) {
    if (vfs_super_is_dying(dir->inode->sb))
        return ERROR_PTR(-EBUSY);

    if (streq(path, "."))
        return vfs_dentry_acquire(dir);

    if (streq(path, ".."))
        return vfs_dentry_parent(dir);

    vfs_dentry* child = vfs_dentry_lookup(dir, path);
    return child ? child : dir->inode->ops->lookup(dir, path);
}

u64 walk_one(vfs_path start, vfs_path* res, path_parts* parts) {
    vfs_mount* mnt = start.mount;
    vfs_dentry* dentry = start.dentry;

    // TODO: handle root, handle global lookups
    if (parts->parts_left == 0)
        return -ENOENT;

    // Try to resolve mountpoint, if we've missed mount due to race condition -
    // just ignore it
    dentry = vfs_dentry_is_mountpoint(dentry) ? vfs_mount_resolve(mnt, dentry)
                                              : dentry;
    // in case of failure fallback to starting dentry
    dentry = dentry ? dentry : start.dentry;

    if (dentry->inode->type != DIRECTORY)
        return -ENOTDIR;

    dentry = lookup(dentry, walk_next_part(parts));
    *res = (vfs_path) {.dentry = dentry, .mount = mnt};
    return IS_ERROR(dentry) ? PTR_ERROR(dentry) : 0;
}

u64 walk_parent(vfs_path start, vfs_path* res, path_parts* parts) {
    vfs_path iter = start;
    vfs_dentry* dentry = iter.dentry;

    *res = start;

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
