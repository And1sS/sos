#include "multiboot.h"
#include "lib/alignment.h"

#define READ_FIELD(field, ptr)                                                 \
    {                                                                          \
        field = *(typeof(field)*) ptr;                                         \
        ptr += sizeof(typeof(field));                                          \
    }

void parse_multiboot_mmap(u64 ptr, multiboot_info* dst) {
    memory_map result;

    READ_FIELD(result.header, ptr)
    READ_FIELD(result.entry_size, ptr)
    READ_FIELD(result.entry_version, ptr);
    result.entries = (memory_map_entry_v0*) ptr;
    result.entries_count =
        (result.header.size - 4 * sizeof(u32)) / result.entry_size;

    dst->mmap = result;
}

void parse_multiboot_elf_symbols(u64 ptr, multiboot_info* dst) {
    elf_sections result;

    READ_FIELD(result.header, ptr)
    READ_FIELD(result.sections_number, ptr)
    READ_FIELD(result.section_size, ptr)
    READ_FIELD(result.shndx, ptr)
    result.sections = (elf64_shdr*) ptr;

    dst->elf_sections = result;
}

void parse_multiboot_tag(u64 ptr, MULTIBOOT_TAG_TYPE type,
                         multiboot_info* dst) {

    switch (type) {
    case MEMORY_MAP:
        parse_multiboot_mmap(ptr, dst);
        break;

    case ELF_SYMBOLS:
        parse_multiboot_elf_symbols(ptr, dst);

    default:
        break;
    }
}

multiboot_info parse_multiboot_info(void* multiboot_info_ptr) {
    multiboot_info result = {.original_struct_addr =
                                 (paddr) V2P(multiboot_info_ptr)};

    u64 start_ptr = (u64) multiboot_info_ptr;
    u64 ptr = start_ptr;

    u32 reserved;

    READ_FIELD(result.size, ptr);
    READ_FIELD(reserved, ptr);

    while (ptr < start_ptr + result.size) {
        u64 tmp = ptr;
        tag_header header;
        READ_FIELD(header, tmp)

        parse_multiboot_tag(ptr, header.type, &result);

        // each entry is 8 byte aligned
        ptr += align_to_upper(header.size, 8);
    }

    return result;
}

void print_multiboot_info(multiboot_info* multiboot_info_ptr) {
    multiboot_info mboot_info = *multiboot_info_ptr;

    println("memory map: ");
    for (u8 i = 0; i < mboot_info.mmap.entries_count; i++) {
        print("t: ");
        print_u32(mboot_info.mmap.entries[i].type);
        print(" s: ");
        print_u64_hex(mboot_info.mmap.entries[i].base_addr);
        print(" e: ");
        print_u64_hex(mboot_info.mmap.entries[i].base_addr
                      + mboot_info.mmap.entries[i].length - 1);
        print(" l: ");
        print_u64(mboot_info.mmap.entries[i].length / 1024);
        println("kb");
    }

    println("");
    println("elf sections: ");
    elf64_shdr name_table =
        mboot_info.elf_sections.sections[mboot_info.elf_sections.shndx];

    for (u64 i = 0; i < mboot_info.elf_sections.sections_number; i++) {
        print("s: ");
        print_u64_hex(mboot_info.elf_sections.sections[i].addr);
        print(" e: ");
        print_u64_hex(mboot_info.elf_sections.sections[i].addr
                      + mboot_info.elf_sections.sections[i].size);
        print(" n: ");
        print((string) (P2V(name_table.addr)
                        + mboot_info.elf_sections.sections[i].name));
        print(" f: ");
        print_u32(mboot_info.elf_sections.sections[i].flags);
        println("");
    }

    u64 kernel_start = 0xFFFFFFFFFFFFFFFF;
    u64 kernel_end = 0;

    for (u32 i = 1; i < mboot_info.elf_sections.sections_number; ++i) {
        elf64_shdr* section = &mboot_info.elf_sections.sections[i];
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
    print_u64_hex((u64) mboot_info.original_struct_addr);
    print(" e: ");
    print_u64_hex((u64) mboot_info.original_struct_addr + mboot_info.size);
}
