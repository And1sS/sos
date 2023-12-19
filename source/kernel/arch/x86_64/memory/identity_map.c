#include "identity_map.h"

#include "../../../memory/pmm.h"
#include "../../../memory/vmm.h"

#define IS_INSIDE(frame, start, end)                                           \
    (((frame) >= (start)) && ((frame) <= (end)))

paddr find_end_of_memory(const multiboot_info* mboot_info);
paddr find_kernel_start(const multiboot_info* mboot_info);
paddr find_kernel_end(const multiboot_info* mboot_info);
bool is_frame_available(const multiboot_info* mboot_info, paddr frame);
bool is_inside_module(const multiboot_info* mboot_info, paddr frame);

void identity_map_ram(const multiboot_info* const mboot_info) {
    paddr memory_end = find_end_of_memory(mboot_info);
    paddr kernel_start = find_kernel_start(mboot_info);
    paddr kernel_end = find_kernel_end(mboot_info);
    paddr mboot_start = mboot_info->original_struct_addr;
    paddr mboot_end = mboot_start + mboot_info->size;

    for (paddr frame = RESERVED_LOWER_PMEM_SIZE; frame < memory_end;
         frame += FRAME_SIZE) {
        u64 page = get_page(P2V(frame));
        if (!(page & 1)) {
            map_page(P2V(frame), frame, 1 | 2 | 4);
        }

        if (!is_frame_available(mboot_info, frame))
            continue;
        if (IS_INSIDE(frame, kernel_start, kernel_end))
            continue;
        if (IS_INSIDE(frame, mboot_start, mboot_end))
            continue;

        free_frame(frame);
    }
}

paddr normalize_kernel_addr(vaddr addr) {
    if (addr < KERNEL_START_VADDR)
        return addr;
    return addr - KERNEL_START_VADDR;
}

paddr find_end_of_memory(const multiboot_info* const mboot_info) {
    paddr memory_end = 0;

    for (u32 i = 0; i < mboot_info->mmap.entries_count; ++i) {
        memory_map_entry_v0* entry = &mboot_info->mmap.entries[i];
        if (entry->type != 1)
            continue;
        paddr entry_end = entry->base_addr + entry->length - 1;
        if (entry_end > memory_end)
            memory_end = entry_end;
    }

    return memory_end;
}

paddr find_kernel_start(const multiboot_info* mboot_info) {
    paddr kernel_start = 0xFFFFFFFFFFFFFFFF;

    for (u32 i = 1; i < mboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &mboot_info->elf_sections.sections[i];
        // kernel sections are actually mix of virtual and physical addresses
        paddr section_addr = normalize_kernel_addr(section->addr);

        if (section_addr < kernel_start)
            kernel_start = section_addr;
    }

    return kernel_start;
}

paddr find_kernel_end(const multiboot_info* mboot_info) {
    paddr kernel_end = 0;

    for (u32 i = 1; i < mboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &mboot_info->elf_sections.sections[i];
        paddr section_addr = normalize_kernel_addr(section->addr);

        // kernel sections are actually mix of virtual and physical addresses
        if (section_addr + section->size > kernel_end)
            kernel_end = section_addr + section->size;
    }

    return kernel_end;
}

bool is_frame_available(const multiboot_info* mboot_info, paddr frame) {
    for (u32 i = 0; i < mboot_info->mmap.entries_count; ++i) {
        memory_map_entry_v0* entry = &mboot_info->mmap.entries[i];
        u64 entry_start = entry->base_addr;
        u64 entry_end = entry_start + entry->length - 1;

        if (IS_INSIDE(frame, entry_start, entry_end) && entry->type == 1) {
            return true;
        }
    }

    return false;
}