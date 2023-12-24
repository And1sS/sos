#include "../interrupts/irq.h"
#include "../lib/kprint.h"
#include "../memory/heap/kheap.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../scheduler/scheduler.h"
#include "../signal/signal.h"
#include "../threading/kthread.h"
#include "../threading/threading.h"
#include "../threading/uthread.h"
#include "arch_init.h"
#include "multiboot.h"

thread* user_thread = NULL;

_Noreturn void kernel_thread() {
    u64 i = 0;
    u64 print = 0;
    bool signaled = false;
    while (1) {
        if (i++ % 1000000 == 0) {
            println("kernel threading!");
            print++;
        }

        if (print > 100 && !signaled) {
            signal_thread(user_thread, SIGTEST);
            signaled = true;
        }
    }
}

void set_up(const multiboot_info* mboot_info) {
    clear_screen();
    println("Starting initialization");

    arch_init(mboot_info);

    kmalloc_init();
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

    // temporary hardcoded loading of first-userspace-program.bin for test
    // which contains only code which start is mapped to 0x1000
    map_page(0x1000, allocate_zeroed_frame(), 1 | 2 | 4);
    memcpy((void*) 0x1000, (void*) P2V(first.mod_start),
           first.mod_end - first.mod_start);

    map_page(0xFFFF, allocate_zeroed_frame(), 1 | 2 | 4);
    map_page(0xFFFF + FRAME_SIZE, allocate_zeroed_frame(), 1 | 2 | 4);
    map_page(0xFFFF + 2 * FRAME_SIZE, allocate_zeroed_frame(), 1 | 2 | 4);
    void* stack = (void*) 0xFFFF;

    user_thread = uthread_create_orphan("test", stack, (uthread_func*) 0x1000);
    thread_start(user_thread);

    kthread_run("kernel-test-thread", kernel_thread);

    local_irq_enable();
    while (true) {
    }
}
