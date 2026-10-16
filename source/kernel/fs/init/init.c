#include "init.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/tar/tar.h"
#include "../file.h"

static u64 copy_tar_file(vfs_path parent, string name, tar_entry* entry) {
    vfs_file* file = vfs_open(parent, name, O_CREAT);
    if (IS_ERROR(file))
        return PTR_ERROR(file);

    u64 copy_res = vfs_write(file, entry->data, tar_parse_size(entry));
    vfs_file_close(file);
    return IS_ERROR(copy_res) ? copy_res : 0;
}

static u64 walk_intermediate_folders(path_parts* parts, vfs_path* parent) {
    vfs_path curr = vfs_root();
    u64 result = 0;

    while (parts->parts_left > 1) {
        vfs_path next;
        string name = path_parts_walk_next(parts);
        result = lookup(curr, &next, name);

        if (result == (u64) -ENOENT) {
            result = vfs_mkdir(curr, name, 0);
            if (IS_ERROR(result))
                goto error;

            result = lookup(curr, &next, name);
        }

        if (IS_ERROR(result))
            goto error;

        vfs_path_release(curr);
        curr = next;
    }

    *parent = curr;
    return result;

error:
    vfs_path_release(curr);
    return result;
}

static void copy_tar_entry(tar_entry* entry) {
    tar_file_type type = tar_parse_type(entry);
    if (type != TAR_FILE && type != TAR_DIRECTORY)
        goto error;

    char path[TAR_NAME_MAX];
    tar_entry_path(entry, path);
    path_parts parts = path_parts_from_path(path);
    if (parts.parts_left == 0)
        goto error;

    vfs_path parent;
    u64 result = walk_intermediate_folders(&parts, &parent);
    if (IS_ERROR(result))
        goto error;

    string name = path_parts_walk_next(&parts);
    result = type == TAR_FILE ? copy_tar_file(parent, name, entry)
                              : vfs_mkdir(parent, name, 0);
    vfs_path_release(parent);

    if (!IS_ERROR(result))
        return;

error:
    panic("Can't create initramfs directory tree, corrupted tar archive");
}

static void initramfs_init(module initramfs_tar) {
    u64 entries = 0;
    tar_entry* entry = (tar_entry*) P2V(initramfs_tar.mod_start);
    tar_entry* end = (tar_entry*) P2V(initramfs_tar.mod_end);

    println("Creating file system tree:");
    while (entry < end && tar_is_valid_entry(entry)) {
        entries++;
        tar_print_entry(entry);
        copy_tar_entry(entry);
        entry = tar_next_entry(entry);
    }

    if (entries == 0)
        panic("Corrupted initfs tar archive");
}

void fs_init(module initramfs_tar) {
    vfs_init();
    initramfs_init(initramfs_tar);
}
