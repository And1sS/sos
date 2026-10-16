#include "file.h"
#include "../error/errno.h"
#include "../error/error.h"

vfs_file* vfs_file_create(vfs_path path, u64 flags) {
    vfs_file* file = kmalloc(sizeof(vfs_file));
    if (!file)
        return ERROR_PTR(-ENOMEM);

    memset(file, 0, sizeof(vfs_file));

    file->path = vfs_path_acquire(path);
    file->ops = path.dentry->inode->file_ops;
    file->flags = flags;
    file->pos = 0;
    file->refc = 0;

    return vfs_file_acquire(file);
}

vfs_file* vfs_file_acquire(vfs_file* file) {
    atomic_increment(&file->refc);
    return file;
}

void vfs_file_release(vfs_file* file) {
    if (atomic_decrement_and_get(&file->refc) != 0)
        return;

    if (file->ops->release)
        file->ops->release(file);

    vfs_path_release(file->path);

    kfree(file);
}

void vfs_file_close(vfs_file* file) {
    // TODO: add file flushing
    vfs_file_release(file);
}