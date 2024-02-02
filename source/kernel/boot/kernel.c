#include "../arch/common/init.h"
#include "../arch/common/vmm.h"
#include "../interrupts/irq.h"
#include "../memory/heap/kheap.h"
#include "../memory/virtual/vmm.h"
#include "../scheduler/scheduler.h"
#include "../threading/kthread.h"
#include "../threading/threading.h"
#include "../threading/uthread.h"
#include "multiboot.h"

void kernel_thread() {
    u64 i = 0;
    u64 print = 0;
    while (print < 1000) {
        if (i++ % 1000000 == 0) {
            println("kernel thread!");
            print++;
        }
    }
}

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
    scheduler_init();
    println("Finished threading initialization!");

    print_multiboot_info(mboot_info);
    println("Finished initialization!");
}

_Noreturn void kernel_main(paddr multiboot_structure) {
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));
    set_up(&multiboot_info);

    module first = get_module_info(&multiboot_info, 0);

    vm_space* kernel_space = vmm_kernel_vm_space();
    println("Kernel vm:");
    vm_space_print(kernel_space);

    vm_space* forked_space = vm_space_fork(kernel_space);

    println("Forked vm:");
    vm_space_print(forked_space);

    vm_area_flags flags = {
        .writable = true, .user_access_allowed = true, .executable = true};

    // temporary hardcoded loading of first-userspace-program.bin for test
    // which contains only code which start is mapped to 0x1000
    vm_space_map_page(forked_space, 0x1000, flags);
    vm_space_map_pages(forked_space, 0xF000, 2, flags);
    println("Forked vm after mapping: ");
    vm_space_print(forked_space);

    void* forked_text_page = vm_space_get_page_view(forked_space, 0x1000);
    memcpy(forked_text_page, (void*) P2V(first.mod_start),
           first.mod_end - first.mod_start);

    vmm_set_vm_space(forked_space);
    thread_start(
        uthread_create_orphan("test", (void*) 0xF000, (uthread_func*) 0x1000));

    kthread_run("kernel_space-test-thread", kernel_thread);

    local_irq_enable();
    while (true) {
    }
}
