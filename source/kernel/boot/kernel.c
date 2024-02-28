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

process* init_process;

void kernel_thread() {
    u64 i = 0;
    u64 printed = 0;

    bool interrupts_enabled = spin_lock_irq_save(&init_process->lock);
    ref_acquire(&init_process->refc);
    spin_unlock_irq_restore(&init_process->lock, interrupts_enabled);

    while (1) {
        if (i++ % 1000000 == 0) {
            print("kernel! Processes revision! Print cnt:");
            print_u64(printed);
            println("");
            printed++;

            if (init_process->finished) {
                ref_release(&init_process->refc);
                return;
            } else if (printed % 100 == 0) {
                process_signal(init_process, SIGINT);
            }
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

    module first = get_module_info(&multiboot_info, 1);

    vm_space* kernel_space = vmm_kernel_vm_space();
    println("Kernel vm:");
    vm_space_print(kernel_space);

    init_process = (process*) kmalloc(sizeof(process));
    process_init(init_process, false);
    vm_space* process_vm = init_process->vm;

    vm_area_flags flags = {
        .writable = true, .user_access_allowed = true, .executable = true};

    // temporary hardcoded loading of test.bin for test, which code and data are
    // within single page, start is mapped to 0x1000, entrypoint is 0x1000
    vm_space_map_page(process_vm, 0x1000, flags);

    void* forked_text_page = vm_space_get_page_view(process_vm, 0x1000);
    memcpy(forked_text_page, (void*) P2V(first.mod_start),
           first.mod_end - first.mod_start);

    uthread* user_thread =
        uthread_create_orphan(init_process, "test", (uthread_func*) 0x1000);

    println("Process vm after mapping: ");
    vm_space_print(process_vm);

    thread_start(user_thread);
    kthread_run("kernel-test-thread", kernel_thread);

    local_irq_enable();
    while (true) {
    }
}
