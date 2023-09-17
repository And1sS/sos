#include "multiboot.h"

#define READ_FIELD(field, type, ptr)                                           \
    {                                                                          \
        field = *(type*) ptr;                                                  \
        ptr += sizeof(type);                                                   \
    }

void parse_multiboot_mmap(u64 multiboot_mmap, parsed_multiboot_info* dst) {
    u64 ptr = multiboot_mmap;
    parsed_memory_map result;

    READ_FIELD(result.type, u32, ptr)
    READ_FIELD(result.size, u32, ptr)
    READ_FIELD(result.entry_size, u32, ptr)
    READ_FIELD(result.entry_version, u32, ptr);
    result.entries = (parsed_memory_map_entry_v0*) ptr;
    result.entries_count = (result.size - 4 * sizeof(u32)) / result.entry_size;

    dst->mmap = result;
}

void parse_multiboot_tag(u64 ptr, MULTIBOOT_TAG_TYPE type,
                         parsed_multiboot_info* dst) {

    switch (type) {
    case MEMORY_MAP:
        parse_multiboot_mmap(ptr, dst);
        break;
    default:
        break;
    }
}

parsed_multiboot_info parse_multiboot_info(void* multiboot_info) {
    parsed_multiboot_info result;

    u64 start_ptr = (u64) multiboot_info;
    u64 ptr = start_ptr;

    u32 total_size;
    u32 reserved;

    READ_FIELD(total_size, u32, ptr);
    READ_FIELD(reserved, u32, ptr);

    while (ptr < start_ptr + total_size) {
        u64 tmp = ptr;
        MULTIBOOT_TAG_TYPE tag_type;
        u32 tag_size;

        READ_FIELD(tag_type, u32, tmp)
        READ_FIELD(tag_size, u32, tmp)

        parse_multiboot_tag(ptr, tag_type, &result);

        // each entry is 8 byte aligned and this alignment is not included in
        // size, so we should add padding
        u32 remainder = tag_size % 8;
        ptr += tag_size + ((remainder != 0) ? (8 - remainder) : 0);
    }

    return result;
}
