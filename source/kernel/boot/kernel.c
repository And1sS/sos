#include "../arch/common/init.h"
#include "../arch/common/vmm.h"
#include "../error/error.h"
#include "../fs/init/init.h"
#include "../interrupts/irq.h"
#include "../lib/alignment.h"
#include "../threading/kthread.h"
#include "../threading/scheduler.h"
#include "../threading/thread_cleaner.h"
#include "../threading/uthread.h"
#include "multiboot.h"

extern process init_process;

static void init_process_init() {
    vfs_path root = vfs_root();
    vfs_file* init_binary = vfs_open(root, "/usr/bin/test.bin", 0);
    if (IS_ERROR(init_binary))
        panic("Can't open init binary");
    vfs_path_release(root);

    // temporary hardcoded loading of test.c for test, start is mapped to
    // 0x1000, entrypoint is 0x1000
    u64 module_size = init_binary->path.dentry->inode->size;
    u64 pages = align_to_upper(module_size, PAGE_SIZE) / PAGE_SIZE;
    u64 offset = 0x1000;

    vm_area_flags flags = {
        .writable = true, .user_access_allowed = true, .executable = true};
    vm_page_mapping_result result =
        vm_space_map_pages_exactly(init_process.vm, offset, pages, flags);
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
    vfs_file_close(init_binary);
    thread_start(uthread_create_orphan(&init_process, "test", NULL,
                                       (uthread_func*) offset));

    vm_space_print(init_process.vm);
}

static void set_up(const multiboot_info* mboot_info) {
    clear_screen();
    println("Starting initialization");
    print_multiboot_info(mboot_info);

    arch_init(mboot_info);
    println("Finished arch initialization!");

    kheap_init();
    vmm_init();
    print("Finished kernel heap initialization! Heap initial size: ");
    print_u64(KHEAP_INITIAL_SIZE);
    println("");

    fs_init(get_module_info(mboot_info, 0));
    println("Finished file system initialization!");

    threading_init();
    processing_init();
    thread_cleaner_init();
    scheduler_init();
    println("Finished threading initialization!");

    init_process_init();
    println("Finished init process initialization!");

    println("Finished initialization!");
}

_Noreturn void kernel_main(paddr multiboot_structure) {
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));

    set_up(&multiboot_info);

    local_irq_enable();
    while (true)
        ;
}
