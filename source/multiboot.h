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
    u64 base_addr;
    u64 length;
    u32 type;
    u32 reserved;
} parsed_memory_map_entry_v0;

typedef struct {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entries_count;
    u32 entry_version;
    parsed_memory_map_entry_v0* entries;
} parsed_memory_map;

typedef struct {
    parsed_memory_map mmap;
} parsed_multiboot_info;

parsed_multiboot_info parse_multiboot_info(void* multiboot_info);

#endif // SOS_MULTIBOOT_H
