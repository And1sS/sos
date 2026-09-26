#include "init.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/tar/tar.h"
#include "../file.h"

static vfs_file* copy_tar_file(vfs_path parent, string name, tar_entry* entry) {
    vfs_file* file = vfs_open(parent, name, O_CREAT);
    if (IS_ERROR(file))
        return file;

    u64 copy_res = vfs_write(file, entry->data, tar_parse_size(entry));
    if (IS_ERROR(copy_res)) {
        vfs_file_close(file);
        return ERROR_PTR(copy_res);
    }

    return file;
}

static u64 create_intermediate_dir(vfs_path parent, string name,
                                   vfs_path* res) {

    vfs_file* created = vfs_mkdir(parent, name, 0);
    if (IS_ERROR(created))
        return PTR_ERROR(created);

    *res = vfs_path_acquire(created->path);
    vfs_file_release(created);
    return 0;
}

// It is safe to do io syscalls on vfs as at this stage vfs tree consists
// on ramfs mounted as tree root, so all of the syscalls are basically RAM
// reads/writes
static void copy_tar_entry(tar_entry* entry) {
    char name[256];
    tar_fill_full_entry_name(entry, name);
    path_parts parts = path_parts_from_path(name);

    vfs_path curr = vfs_root();
    // create folders above our entry
    while (parts.parts_left > 1) {
        vfs_path res;
        u64 walk_res = walk_one(curr, &res, &parts);
        if (walk_res == (u64) -ENOENT)
            walk_res = create_intermediate_dir(curr, parts.part, &res);

        if (IS_ERROR(walk_res))
            panic("Can't create initramfs directory tree");

        vfs_path_release(curr);
        curr = res;
    }

    part_walk_next(&parts);

    vfs_file* file = ERROR_PTR(-EINVAL);
    tar_file_type tar_entry_type = tar_parse_type(entry);
    if (tar_entry_type == TAR_NORMAL_FILE)
        file = copy_tar_file(curr, parts.part, entry);
    else if (tar_entry_type == TAR_DIRECTORY)
        file = vfs_mkdir(curr, parts.part, 0);

    if (IS_ERROR(file))
        panic("Can't create initramfs directory tree, corrupted tar archive");

    vfs_path_release(curr);
    vfs_file_close(file);
}

static void initramfs_init(module initramfs_tar) {
    u64 entries = 0;
    tar_entry* entry = (tar_entry*) P2V(initramfs_tar.mod_start);
    tar_entry* end = (tar_entry*) P2V(initramfs_tar.mod_end);

    println("Creating file system tree:");
    while (entry <= end && tar_is_valid_entry(entry)) {
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
