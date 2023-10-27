#include "arch_init.h"
#include "lib/types.h"
#include "multiboot.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "interrupts/interrupts.h"
#include "vga_print.h"

extern void resume_thread();

_Noreturn void test_func() {
    while (true) {
        println("hello from thread func!");
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

    init_scheduler();
    disable_interrupts();

    thread thrd;
    init_thread(&thrd, "test-thread", test_func);

    __asm__ volatile("movq %0, %%rsp\n"
                     "jmp resume_thread"
                     :
                     : "g"(thrd.rsp)
                     : "memory");

    while (true) {
    }
}
