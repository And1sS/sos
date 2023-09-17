#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "types.h"
#include "vga_print.h"

void init(void);

#include "multiboot.h"

_Noreturn void kernel_main(void* multiboot_structure) {
    init();

    clear_screen();

    parsed_multiboot_info ms = parse_multiboot_info(multiboot_structure);
    println("memory map: ");
    for (u8 i = 0; i < ms.mmap.entries_count; i++) {
        print("t: ");
        print_u32(ms.mmap.entries[i].type);
        print(" s: ");
        print_u64_hex(ms.mmap.entries[i].base_addr);
        print(" e: ");
        print_u64_hex(ms.mmap.entries[i].base_addr + ms.mmap.entries[i].length);
        print(" l: ");
        print_u64(ms.mmap.entries[i].length / 1024 / 1024);
        println("mb");
    }

    println("");
    println("elf sections: ");
    elf64_shdr name_table = ms.elf_sections.sections[ms.elf_sections.shndx];

    for (int i = 0; i < ms.elf_sections.sections_number; i++) {
        print("s: ");
        print_u64_hex(ms.elf_sections.sections[i].addr);
        print(" e: ");
        print_u64_hex(ms.elf_sections.sections[i].addr
                      + ms.elf_sections.sections[i].size);
        print(" n: ");
        print(
            (const char*) (name_table.addr + ms.elf_sections.sections[i].name));
        print(" f: ");
        print_u32(ms.elf_sections.sections[i].flags);
        println("");
    }

    println("");
    u64 kernel_start = 0xFFFFFFFFFFFFFFFF, kernel_end = 0;
    for (int i = 1; i < ms.elf_sections.sections_number; ++i) {
        elf64_shdr* section = &ms.elf_sections.sections[i];
        if (section->addr < kernel_start)
            kernel_start = section->addr;
        if (section->addr + section->size > kernel_end)
            kernel_end = section->addr + section->size;
    }

    print("kernel s: ");
    print_u64_hex(kernel_start);
    print(" e: ");
    print_u64_hex(kernel_end);
    println("");

    print("multiboot s: ");
    print_u64_hex((u64) multiboot_structure);
    print(" e: ");
    print_u64_hex((u64) multiboot_structure + ms.size);

    while (true) {
    }
}

void init(void) {
    init_gdt();
    init_timer();
    init_idt();
}