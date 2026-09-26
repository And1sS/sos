#include "init.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../fs/file.h"
#include "../lib/alignment.h"
#include "../lib/tar/tar.h"
#include "../threading/process.h"
#include "../threading/thread.h"
#include "../threading/uthread.h"

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
        panic("Can't create initramfs directory tree");

    vfs_path_release(curr);
    vfs_file_close(file);
}

void set_up_init_fs(module initramfs_module) {
    u64 entries = 0;
    tar_entry* entry = (tar_entry*) P2V(initramfs_module.mod_start);
    tar_entry* end = (tar_entry*) P2V(initramfs_module.mod_end);

    while (entry <= end && tar_is_valid_entry(entry)) {
        entries++;

        print(entry->header.name);
        print(" ");
        print_u64(tar_parse_size(entry));
        println("");

        copy_tar_entry(entry);
        entry = tar_next_entry(entry);
    }

    if (entries == 0)
        panic("Corrupted initfs tar archive");
}

extern process init_process;

void set_up_init_process() {
    vfs_file* init_binary = vfs_open(vfs_root(), "/usr/bin/test.bin", 0);
    if (IS_ERROR(init_binary))
        panic("Can't open init binary");

    vm_area_flags flags = {
        .writable = true, .user_access_allowed = true, .executable = true};

    // temporary hardcoded loading of test.c for test, start is mapped to
    // 0x1000, entrypoint is 0x1000
    u64 module_size = init_binary->path.dentry->inode->size;
    u64 pages = align_to_upper(module_size, PAGE_SIZE) / PAGE_SIZE;
    u64 offset = 0x1000;

    vm_page_mapping_result result =
        vm_space_map_pages_exactly(init_process.vm, 0x1000, pages, flags);
    if (result != SUCCESS)
        panic("Couldn't map enough pages for init process");

    u16 chunk_size = 256;
    u8 buffer[chunk_size];

    for (u64 i = 0; i < pages; i++) {
        u64 page_base = offset + i * PAGE_SIZE;

        for (u64 j = 0; j < PAGE_SIZE / chunk_size; j++) {
            u64 read = vfs_read(init_binary, buffer, chunk_size);
            if (IS_ERROR(read))
                panic("Couldn't read init binary");

            if (read == 0)
                goto out;

            void* page = vm_space_get_page_view(init_process.vm, page_base);
            void* dst = (void*) ((u64) page + j * chunk_size);
            memcpy(dst, buffer, read);
        }
    }

out:

    thread_start(uthread_create_orphan(&init_process, "test", NULL,
                                       (uthread_func*) offset));

    vm_space_print(init_process.vm);
}