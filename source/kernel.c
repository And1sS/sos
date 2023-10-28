#include "arch_init.h"
#include "interrupts/interrupts.h"
#include "lib/types.h"
#include "multiboot.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "vga_print.h"

thread t1;
thread t2;

_Noreturn void t1_func() {
    int i = 0;
    while (true) {
        print_u32(i++);
        switch_context(&t2);
    }
}

_Noreturn void t2_func() {
    while (true) {
        println("       hello  ");
        switch_context(&t1);
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

    init_thread(&t1, "test-thread-1", t1_func);
    init_thread(&t2, "test-thread-2", t2_func);
    resume_thread(&t1);

    while (true) {
    }
}
