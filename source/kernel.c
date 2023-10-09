#include "arch_init.h"
#include "lib/types.h"
#include "multiboot.h"
#include "vga_print.h"

_Noreturn void kernel_main(paddr multiboot_structure) {
    clear_screen();
    println("Starting initialization");
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));
    arch_init(&multiboot_info);

    print_multiboot_info(&multiboot_info);
    println("Finished initialization!");

    while (true) {
    }
}
