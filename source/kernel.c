#include "lib/types.h"
#include "memory/pmm.h"
#include "multiboot.h"
#include "timer.h"
#include "vga_print.h"
#include "x86_64/gdt.h"
#include "x86_64/interrupts/idt.h"

void init(multiboot_info* multiboot_info);

_Noreturn void kernel_main(paddr multiboot_structure) {
    multiboot_info multiboot_info =
        parse_multiboot_info((void*) P2V(multiboot_structure));
    init(&multiboot_info);

    clear_screen();
    print_multiboot_info(&multiboot_info);

    while (true) {
    }
}

void init(multiboot_info* multiboot_info) {
    init_gdt();
    init_pmm(multiboot_info);

    init_timer();
    init_idt();
}
