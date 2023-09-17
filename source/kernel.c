#include "multiboot.h"
#include "physical_memory_allocator.h"
#include "timer.h"
#include "types.h"
#include "vga_print.h"
#include "x86_64/gdt.h"
#include "x86_64/idt.h"

void init(multiboot_info* multiboot_info);

_Noreturn void kernel_main(void* multiboot_structure) {
    multiboot_info multiboot_info = parse_multiboot_info(multiboot_structure);
    init(&multiboot_info);

    clear_screen();

    for (u64 i = 0; i < 10; ++i) {
        void* allocated_frame = allocate_frame();
        print("allocated frame: ");
        print_u64_hex((u64) allocated_frame);
        println("");
    }

    while (true) {
    }
}

void init(multiboot_info* multiboot_info) {
    init_gdt();
    init_physical_allocator(multiboot_info);

    init_timer();
    init_idt();
}