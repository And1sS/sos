#include "init.h"
#include "../error/errno.h"
#include "../error/error.h"
#include "../fs/file.h"
#include "../lib/alignment.h"
#include "../lib/tar/tar.h"
#include "../threading/process.h"
#include "../threading/thread.h"
#include "../threading/uthread.h"

static void copy_tar_entry(tar_entry* entry) {
    char name[256];
    tar_fill_full_entry_name(entry, name);

    path_parts parts = path_parts_from_path(name);

    vfs_path res;
    vfs_path curr = vfs_root();

    // create folders above our entry
    while (parts.parts_left > 1) {
        u64 walk_res = walk_one(curr, &res, &parts);

        if (IS_ERROR(walk_res) && walk_res != (u64) -ENOENT)
            goto error;
        else if (IS_ERROR(walk_res)) {
            vfs_file* created = vfs_mkdir(curr, parts.part, 0);
            if (IS_ERROR(created))
                goto error;

            res = vfs_path_acquire(created->path);
            vfs_file_release(created);
        }

        vfs_path_release(curr);
        curr = res;
    }

    part_walk_next(&parts);

    vfs_file* file;
    switch (tar_parse_type(entry)) {
    case TAR_NORMAL_FILE:
        file = vfs_open(curr, parts.part, O_CREAT);
        if (IS_ERROR(file))
            goto error;

        u64 copy_res = vfs_write(file, entry->data, tar_parse_size(entry));
        if (IS_ERROR(copy_res))
            goto error;
        break;

    case TAR_DIRECTORY:
        file = vfs_mkdir(curr, parts.part, 0);
        break;
    default:
        goto error;
    }

    if (IS_ERROR(file))
        goto error;

    return;

error:
    panic("Can't create initramfs");
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