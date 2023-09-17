#include "gdt.h"
#include "idt.h"
#include "multiboot.h"
#include "physical_memory_allocator.h"
#include "timer.h"
#include "types.h"
#include "vga_print.h"

void init(parsed_multiboot_info* multiboot_info);

_Noreturn void kernel_main(void* multiboot_structure) {
    parsed_multiboot_info ms = parse_multiboot_info(multiboot_structure);
    init(&ms);

    clear_screen();

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


    while (true) {
    }
}

void init(parsed_multiboot_info* multiboot_info) {
    init_gdt();
    init_timer();
    init_idt();

    u64 kernel_start = 0xFFFFFFFFFFFFFFFF, kernel_end = 0;
    for (u32 i = 1; i < multiboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &multiboot_info->elf_sections.sections[i];
        if (section->addr < kernel_start)
            kernel_start = section->addr;
        if (section->addr + section->size > kernel_end)
            kernel_end = section->addr + section->size;
    }

    init_physical_allocator(&multiboot_info->mmap, kernel_start, kernel_end,
                            (u64) multiboot_info,
                            (u64) multiboot_info + multiboot_info->size);
}