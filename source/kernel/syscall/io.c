#include "../error/error.h"
#include "../fs/path.h"
#include "../threading/process.h"
#include "syscall.h"

u64 sys_open(string __user path, u64 flags) {
    string path_copy = copy_string_from_user(path, PATH_MAX);
    if (IS_ERROR(path_copy))
        return PTR_ERROR(path_copy);

    vfs_path start =
        path_copy[0] == '/' ? process_root() : process_working_directory();
    vfs_file* file = vfs_open(start, path_copy, flags);

    vfs_path_release(start);
    strfree(path_copy);

    if (IS_ERROR(file))
        return PTR_ERROR(file);

    u64 result = process_add_file(file);
    if (IS_ERROR(result))
        vfs_file_release(file);

    return result;
}

u64 sys_openat(u64 fd, string __user path, u64 flags) {
    string path_copy = copy_string_from_user(path, PATH_MAX);
    if (IS_ERROR(path_copy))
        return PTR_ERROR(path_copy);

    vfs_file* at = process_get_file(fd);
    if (IS_ERROR(at)) {
        strfree(path_copy);
        return PTR_ERROR(at);
    }

    vfs_path start =
        path_copy[0] == '/' ? process_root() : vfs_path_acquire(at->path);
    vfs_file* file = vfs_open(start, path_copy, flags);

    vfs_file_release(at);
    vfs_path_release(start);
    strfree(path_copy);

    if (IS_ERROR(file))
        return PTR_ERROR(file);

    u64 result = process_add_file(file);
    if (IS_ERROR(result))
        vfs_file_release(file);

    return result;
}

u64 sys_close(u64 fd) {
    vfs_file* file = process_remove_file(fd);
    if (IS_ERROR(file))
        return PTR_ERROR(file);

    vfs_file_close(file);
    return 0;
}

u64 sys_read(u64 fd, void* __user buf, u64 size) {
    vfs_file* file = process_get_file(fd);
    if (IS_ERROR(file))
        return PTR_ERROR(file);

    u64 result = vfs_read(file, buf, size);
    vfs_file_release(file);

    return result;
}

u64 sys_write(u64 fd, void* __user buf, u64 size) {
    vfs_file* file = process_get_file(fd);
    if (IS_ERROR(file))
        return PTR_ERROR(file);

    u64 result = vfs_write(file, buf, size);
    vfs_file_release(file);

    return result;
}

u64 sys_seek(u64 fd, u64 offset, u64 whence) {
    vfs_file* file = process_get_file(fd);
    if (IS_ERROR(file))
        return PTR_ERROR(file);

    u64 result = vfs_seek(file, offset, whence);
    vfs_file_release(file);

    return result;
}

u64 sys_chroot(string __user path) {
    string path_copy = copy_string_from_user(path, PATH_MAX);
    if (IS_ERROR(path_copy))
        return PTR_ERROR(path_copy);

    vfs_path new_root;
    vfs_path start = process_root(); // chroot path is always relative to root
    path_parts parts = path_parts_from_path(path_copy);

    u64 error = walk(start, &new_root, &parts);
    vfs_path_release(start);
    strfree(path_copy);

    if (IS_ERROR(error))
        return error;

    process_change_root(new_root);
    vfs_path_release(new_root);
    return 0;
}

u64 sys_chdir(string __user path) {
    string path_copy = copy_string_from_user(path, PATH_MAX);
    if (IS_ERROR(path_copy))
        return PTR_ERROR(path_copy);

    vfs_path new_workdir;
    vfs_path start =
        path_copy[0] == '/' ? process_root() : process_working_directory();
    path_parts parts = path_parts_from_path(path_copy);

    u64 error = walk(start, &new_workdir, &parts);
    vfs_path_release(start);
    strfree(path_copy);

    if (IS_ERROR(error))
        return error;

    process_change_working_directory(new_workdir);
    vfs_path_release(new_workdir);
    return 0;
}
