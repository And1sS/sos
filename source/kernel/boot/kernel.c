#include "../interrupts/irq.h"
#include "../lib/kprint.h"
#include "arch_init.h"
#include "multiboot.h"

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

    local_irq_enable();
    while (true) {
    }
}
