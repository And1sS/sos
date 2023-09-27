#ifndef SOS_MULTIBOOT_H
#define SOS_MULTIBOOT_H

#include "types.h"

typedef enum {
    EMPTY = 0,
    BOOT_COMMAND_LINE = 1,
    BOOT_LOADER_NAME = 2,
    MODULES = 3,
    BASIC_MEMORY_INFORMATION = 4,
    BIOS_BOOT_DEVICE = 5,
    MEMORY_MAP = 6,
    VBE_INFO = 7,
    FRAMEBUFFER_INFO = 8,
    ELF_SYMBOLS = 9,
    APM_TABLE = 10,
    EFI_32_BIT_SYSTEM_TABLE_POINTER = 11,
    EFI_64_BIT_SYSTEM_TABLE_POINTER = 12,
    SMBIOS_TABLES = 13,
    ACPI_OLD_RSDP = 14,
    ACPI_NEW_RSDP = 15,
    NETWORKING_INFORMATION = 16,
    EFI_MEMORY_MAP = 17,
    EFI_BOOT_SERVICES_NOT_TERMINATED = 18,
    EFI_32_BIT_IMAGE_HANDLE_POINTER = 19,
    EFI_64_BIT_IMAGE_HANDLE_POINTER = 20,
    IMAGE_LOAD_BASE_PHYSICAL_ADDRESS = 21
} MULTIBOOT_TAG_TYPE;

typedef struct {
    u32 type;
    u32 size;
} tag_header;

typedef struct {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 reserved;
} memory_map_entry_v0;

typedef struct {
    tag_header header;
    u32 entry_size;
    u32 entries_count;
    u32 entry_version;
    memory_map_entry_v0* entries;
} memory_map;

// https://docs.oracle.com/cd/E19683-01/816-1386/6m7qcoblj/index.html#chapter6-28341
typedef struct {
    u32 name;
    u32 type;
    u64 flags;
    u64 addr;
    u64 offset;
    u64 size;
    u32 link;
    u32 info;
    u64 addralign;
    u64 entsize;
} elf64_shdr;

// https://forum.osdev.org/viewtopic.php?f=1&t=33268
typedef struct {
    tag_header header;
    u32 sections_number;
    u32 section_size;
    u32 shndx;
    elf64_shdr* sections;
} elf_sections;

typedef struct {
    void* original_struct_addr;
    u32 size;
    memory_map mmap;
    elf_sections elf_sections;
} multiboot_info;

multiboot_info parse_multiboot_info(void* multiboot_info_ptr);

#endif // SOS_MULTIBOOT_H
