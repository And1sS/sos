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

vfs_path vfs_path_acquire(vfs_path path) {
    vfs_dentry_acquire(path.dentry);
    vfs_mount_acquire(path.mount);

    return path;
}

void vfs_path_release(vfs_path path) {
    vfs_dentry_release(path.dentry);
    vfs_mount_release(path.mount);
}

static u64 part_length(string path) {
    u64 len = 0;
    for (; path[len] != '/' && path[len] != '\0' && len < NAME_MAX - 1; len++)
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

static u64 lookup_current(vfs_path start, vfs_path* res) {
    *res = vfs_path_acquire(start);
    return 0;
}

static u64 lookup_parent(vfs_path start, vfs_path* res) {
    vfs_path resolved_start;
    vfs_mount_walk_up(start, &resolved_start);

    res->dentry = vfs_dentry_parent(resolved_start.dentry);
    res->mount = vfs_mount_acquire(resolved_start.mount);
    vfs_path_release(resolved_start);

    return 0;
}

vfs_dentry* lookup_child(vfs_dentry* dentry, string name) {
    if (dentry->inode->type != DIRECTORY)
        return ERROR_PTR(-ENOTDIR);

    vfs_dentry* child = vfs_dentry_lookup(dentry, name);
    return child ? child : dentry->inode->ops->lookup(dentry, name);
}

u64 lookup(vfs_path start, vfs_path* res, string path) {
    vfs_dentry* dentry = start.dentry;
    if (vfs_super_is_dying(dentry->inode->sb))
        return -EBUSY;

    if (streq(path, "."))
        return lookup_current(start, res);

    if (streq(path, ".."))
        return lookup_parent(start, res);

    vfs_dentry* child = lookup_child(dentry, path);
    if (IS_ERROR(child))
        return PTR_ERROR(child);

    vfs_path child_path = {.dentry = child, .mount = start.mount};
    vfs_mount_walk_down(child_path, res);
    vfs_dentry_release(child);
    return 0;
}

u64 walk_one(vfs_path start, vfs_path* res, path_parts* parts) {
    if (parts->parts_left == 0)
        return -ENOENT;

    return lookup(start, res, walk_next_part(parts));
}

u64 walk_parent(vfs_path start, vfs_path* res, path_parts* parts) {
    *res = start;
    vfs_path_acquire(start);

    while (true) {
        vfs_path curr = *res;

        if (parts->parts_left == 1)
            return 0;

        vfs_dentry* dentry = curr.dentry;
        vfs_inode_lock_shared(dentry->inode);
        u64 error = walk_one(curr, res, parts);
        vfs_inode_unlock_shared(dentry->inode);

        vfs_path_release(curr);

        if (IS_ERROR(error))
            return error;
    }
}

u64 walk(vfs_path start, vfs_path* res, path_parts* parts) {
    vfs_path parent;
    u64 error = walk_parent(start, &parent, parts);
    if (IS_ERROR(error))
        return error;

    vfs_inode_lock_shared(parent.dentry->inode);
    error = walk_one(parent, res, parts);
    vfs_inode_unlock_shared(parent.dentry->inode);

    vfs_path_release(parent);

    return error;
}
