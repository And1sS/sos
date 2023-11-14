#include "arch_init.h"
#include "interrupts/irq.h"
#include "lib/types.h"
#include "multiboot.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "vga_print.h"

#define PRINT_THRESHOLD 1000000
#define PRINT_TIMES 2000
thread t2;

void t2_func() {
    u64 i = 0;
    u64 printed = 0;

    while (printed <= PRINT_TIMES) {
        if (i++ % PRINT_THRESHOLD == 0) {
            println("thread 22222222222!");
            printed++;
        }
    }
}

void t1_func() {
    thread_init(&t2, "thread-2", t2_func);
    schedule_thread_start(&t2);

    u64 i = 0;
    u64 printed = 0;

    while (printed <= 2 * PRINT_TIMES) {
        if (i++ % PRINT_THRESHOLD == 0) {
            println("thread 1!");
            printed++;
        }
    }
}

_Noreturn void kernel_main(paddr multiboot_structure) {
    init_console();

    clear_screen();
    println("Starting initialization");
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));
    arch_init(&multiboot_info);

    print_multiboot_info(&multiboot_info);
    println("Finished initialization!");

    thread t1;
    thread_init(&t1, "test-thread-1", t1_func);
    schedule_thread_start(&t1);

    local_irq_enable();
    while (true) {
    }
}
