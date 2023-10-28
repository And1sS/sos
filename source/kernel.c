#include "arch_init.h"
#include "interrupts/interrupts.h"
#include "lib/types.h"
#include "multiboot.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"
#include "vga_print.h"


_Noreturn void t1_func() {
    while (true) {
        println("thread 1!");
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
    init_thread(&t1, "test-thread-1", t1_func);
    add_thread(&t1);

    resume_thread(&t1);
    while (true) {
    }
}
