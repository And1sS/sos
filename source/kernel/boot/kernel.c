#include "../arch/common/init.h"
#include "../arch/common/vmm.h"
#include "../interrupts/irq.h"
#include "../memory/heap/kheap.h"
#include "../memory/virtual/vmm.h"
#include "../threading/kthread.h"
#include "../threading/scheduler.h"
#include "../threading/thread_cleaner.h"
#include "../threading/threading.h"
#include "../threading/uthread.h"
#include "multiboot.h"

void set_up(const multiboot_info* mboot_info) {
    clear_screen();
    println("Starting initialization");

    arch_init(mboot_info);

    kheap_init();
    vmm_init();

    print("Finished kernel heap initialization! Heap initial size: ");
    print_u64(KHEAP_INITIAL_SIZE);
    println("");

    threading_init();
    processing_init();
    thread_cleaner_init();
    scheduler_init();

    println("Finished threading initialization!");

    print_multiboot_info(mboot_info);
    println("Finished initialization!");
}

_Noreturn void kernel_main(paddr multiboot_structure) {
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));
    set_up(&multiboot_info);

    module first = get_module_info(&multiboot_info, 1);

    vm_space* kernel_space = vmm_kernel_vm_space();
    println("Kernel vm:");
    vm_space_print(kernel_space);

    process* init = create_user_init_process();
    vm_area_flags flags = {
        .writable = true, .user_access_allowed = true, .executable = true};

    // temporary hardcoded loading of test.bin for test, which code and data are
    // within single page, start is mapped to 0x1000, entrypoint is 0x1000
    vm_space_map_page(init->vm, 0x1000, flags);
    void* user_text = vm_space_get_page_view(init->vm, 0x1000);
    memcpy(user_text, (void*) P2V(first.mod_start),
           first.mod_end - first.mod_start);

    thread_start(
        uthread_create_orphan(init, "test", NULL, (uthread_func*) 0x1000));

    vm_space_print(init->vm);
    local_irq_enable();
    while (true) {
    }
}
