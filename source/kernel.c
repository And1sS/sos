#include "arch_init.h"
#include "interrupts/irq.h"
#include "lib/types.h"
#include "multiboot.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "synchronization/completion.h"
#include "vga_print.h"

#define PRINT_THRESHOLD 1000000
#define PRINT_TIMES 2000
#define CONDITION_MET_PRINT_THRESHOLD 1000

/*
 * Modeled flow:
 * t1: starts and creates/starts t2
 * t2: starts and goes into sleep
 *
 * t1: prints 1000 times and signals condition
 * t2: awakes
 * t1 and t2: print 2000 times
 *
 * t2: dies
 * t1: prints 1000 times
 * t1: dies
 */
completion comp;

void t2_func() {
    u64 i = 0, printed = 0;

    completion_wait(&comp);
    while (printed <= PRINT_TIMES) {
        if (i++ % PRINT_THRESHOLD == 0) {
            println("thread 22222222222!");
            printed++;
        }
    }
}

void t1_func() {
    u64 i = 0, printed = 0;
    bool signaled = false;

    completion_init(&comp);
    thread_start(thread_create("thread-2", t2_func));

    while (printed <= 2 * PRINT_TIMES) {
        if (i++ % PRINT_THRESHOLD == 0) {
            println("thread 1!");
            printed++;
        }

        if (printed >= CONDITION_MET_PRINT_THRESHOLD && !signaled) {
            signaled = true;
            completion_complete(&comp);
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

    thread_start(thread_create("test-thread-1", t1_func));

    local_irq_enable();
    while (true) {
    }
}
