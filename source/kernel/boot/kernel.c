#include "../arch/common/init.h"
#include "../arch/common/vmm.h"
#include "../fs/dcache/dentry.h"
#include "../fs/file.h"
#include "../interrupts/irq.h"
#include "../lib/alignment.h"
#include "../threading/kthread.h"
#include "../threading/scheduler.h"
#include "../threading/thread_cleaner.h"
#include "../threading/uthread.h"
#include "init.h"
#include "multiboot.h"

extern process init_process;

void set_up(const multiboot_info* mboot_info) {
    clear_screen();
    println("Starting initialization");

    arch_init(mboot_info);

    kheap_init();
    vmm_init();

    print("Finished kernel heap initialization! Heap initial size: ");
    print_u64(KHEAP_INITIAL_SIZE);
    println("");

    vfs_init();
    println("Finished vfs initialization!");

    threading_init();
    processing_init();

    thread_cleaner_init();
    scheduler_init();

    println("Finished threading initialization!");

    print_multiboot_info(mboot_info);
    println("Finished initialization!");

    println("Finished vfs initialization");
}

_Noreturn void kernel_main(paddr multiboot_structure) {
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));

    set_up(&multiboot_info);
    set_up_init_fs(get_module_info(&multiboot_info, 0));
    set_up_init_process();

    local_irq_enable();
    while (true) {
    }
}
