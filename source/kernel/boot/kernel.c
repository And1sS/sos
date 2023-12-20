#include "../interrupts/irq.h"
#include "../lib/kprint.h"
#include "../memory/heap/kheap.h"
#include "../scheduler/scheduler.h"
#include "../threading/uthread.h"
#include "arch_init.h"
#include "multiboot.h"

#define PRINT_THRESHOLD 1000000
#define PRINT_TIMES 2
#define CONDITION_MET_PRINT_THRESHOLD 1

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
DECLARE_COMPLETION(comp);

u64 t2_func() {
    u64 i = 0, printed = 0;

    completion_wait(&comp);
    while (printed <= PRINT_TIMES) {
        if (i++ % PRINT_THRESHOLD == 0) {
            println("thread 22222222222!");
            printed++;
        }
    }

    return 12;
}

u64 t1_func() {
    u64 i = 0, printed = 0;
    bool signaled = false;

    uthread* t2 =
        uthread_create("thread-2", kmalloc_aligned(8192, 4096), t2_func);
    thread_start(t2);

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

    u64 t2_exit_code = thread_join(t2);
    print("child thread finished with exit code: ");
    print_u64(t2_exit_code);
    println("");

    return 0;
}

_Noreturn void kernel_main(paddr multiboot_structure) {
    clear_screen();
    println("Starting initialization");
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));
    arch_init(&multiboot_info);

    print_multiboot_info(&multiboot_info);
    module mod = get_module_info(&multiboot_info, 0);
    print_module_info(&mod);
    println("Finished initialization!");

//    for (u64 i = 0; i < mod.mod_end - mod.mod_start; ++i) {
//        print_char(*(char*)P2V(mod.mod_start + i));
//
//        if (i % (80 * 25) == 0) {
//            println("");
//        }
//    }

    //    uthread* t1 = uthread_create_orphan("test-thread-1",
    //                                        kmalloc_aligned(8192, 4096),
    //                                        t1_func);
    // thread_start(t1);

    local_irq_enable();
    while (true) {
    }
}
