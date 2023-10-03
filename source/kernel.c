#include "lib/types.h"
#include "memory/pmm.h"
#include "multiboot.h"
#include "timer.h"
#include "vga_print.h"
#include "x86_64/gdt.h"
#include "x86_64/idt.h"

void init(multiboot_info* multiboot_info);

_Noreturn void kernel_main(void* multiboot_structure) {
    multiboot_info multiboot_info = parse_multiboot_info(multiboot_structure);

    clear_screen();

    println("memory map: ");
    for (u8 i = 0; i < multiboot_info.mmap.entries_count; i++) {
        print("t: ");
        print_u32(multiboot_info.mmap.entries[i].type);
        print(" s: ");
        print_u64_hex(multiboot_info.mmap.entries[i].base_addr);
        print(" e: ");
        print_u64_hex(multiboot_info.mmap.entries[i].base_addr
                      + multiboot_info.mmap.entries[i].length - 1);
        print(" l: ");
        print_u64(multiboot_info.mmap.entries[i].length / 1024);
        println("kb");
    }

//    println("");
//    println("elf sections: ");
//    elf64_shdr name_table =
//        multiboot_info.elf_sections.sections[multiboot_info.elf_sections.shndx];
//
//    for (u64 i = 0; i < multiboot_info.elf_sections.sections_number; i++) {
//        print("s: ");
//        print_u64_hex(multiboot_info.elf_sections.sections[i].addr);
//        print(" e: ");
//        print_u64_hex(multiboot_info.elf_sections.sections[i].addr
//                      + multiboot_info.elf_sections.sections[i].size);
//        print(" n: ");
//        print(
//            (const char*) (name_table.addr +
//                           multiboot_info.elf_sections.sections[i].name));
//        print(" f: ");
//        print_u32(multiboot_info.elf_sections.sections[i].flags);
//        println("");
//    }

//    u64 kernel_start = 0xFFFFFFFFFFFFFFFF;
//    u64 kernel_end = 0;
//
//    for (u32 i = 1; i < multiboot_info.elf_sections.sections_number; ++i) {
//        elf64_shdr* section = &multiboot_info.elf_sections.sections[i];
//        if (section->addr < kernel_start)
//            kernel_start = section->addr;
//        if (section->addr + section->size > kernel_end)
//            kernel_end = section->addr + section->size;
//    }
//
//    print("kernel s: ");
//    print_u64_hex(kernel_start);
//    print(" e: ");
//    print_u64_hex(kernel_end);
//    println("");
//
//    print("multiboot s: ");
//    print_u64_hex((u64) multiboot_structure);
//    print(" e: ");
//    print_u64_hex((u64) multiboot_structure + multiboot_info.size);
//
//    init(&multiboot_info);

    while (true) {
    }
}

void init(multiboot_info* multiboot_info) {
    init_gdt();
    init_pmm(multiboot_info);

    init_timer();
    init_idt();
}