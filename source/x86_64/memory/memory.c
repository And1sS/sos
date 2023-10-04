#include "../../memory/memory.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"

extern u64 p1_table;

u64 normalize_kernel_addr(u64 addr) {
    if (addr < KERNEL_START_VADDR)
        return addr;
    return addr - KERNEL_START_VADDR;
}

#define IS_INSIDE(frame, start, end)                                           \
    (((frame) >= (start)) && ((frame) <= (end)))

void init_memory(const multiboot_info* mboot_info) {
    u64 memory_end = 0;
    u64 kernel_start = 0xFFFFFFFFFFFFFFFF;
    u64 kernel_end = 0;

    for (u32 i = 1; i < mboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &mboot_info->elf_sections.sections[i];
        u64 section_addr = normalize_kernel_addr(section->addr);

        if (section_addr < kernel_start)
            kernel_start = section_addr;
        if (section_addr + section->size > kernel_end)
            kernel_end = section_addr + section->size;
    }

    for (u32 i = 0; i < mboot_info->mmap.entries_count; ++i) {
        memory_map_entry_v0* entry = &mboot_info->mmap.entries[i];
        if (entry->type != 1)
            continue;
        u64 entry_end = entry->base_addr + entry->length - 1;
        if (entry_end > memory_end)
            memory_end = entry_end;
    }

    u64 mboot_start = mboot_info->original_struct_addr;
    u64 mboot_end = mboot_start + mboot_info->size;

    for (u64 frame = RESERVED_LOWER_PMEM_SIZE; frame < memory_end;
         frame += FRAME_SIZE) {
        u64 page = get_page(P2V(frame));
        if (!(page & 1)) {
            map_page(P2V(frame), frame, 1 | 2 | 4);
        }

        bool available = false;
        for (u32 i = 0; i < mboot_info->mmap.entries_count; ++i) {
            memory_map_entry_v0* entry = &mboot_info->mmap.entries[i];
            u64 entry_start = entry->base_addr;
            u64 entry_end = entry_start + entry->length - 1;

            if (IS_INSIDE(frame, entry_start, entry_end)) {
                available = entry->type == 1;
            }
        }

        if (!available)
            continue;
        if (IS_INSIDE(frame, kernel_start, kernel_end))
            continue;
        if (IS_INSIDE(frame, mboot_start, mboot_end))
            continue;

        free_frame(frame);
    }

    print("Finished memory mapping! Free frames: ");
    print_u64(get_available_frames_count());
    println("");
}
