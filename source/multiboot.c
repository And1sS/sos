#include "multiboot.h"

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
    multiboot_info result;

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

        // each entry is 8 byte aligned and this alignment is not included in
        // size, so we should add padding
        u32 remainder = header.size % 8;
        ptr += header.size + ((remainder != 0) ? (8 - remainder) : 0);
    }

    return result;
}
