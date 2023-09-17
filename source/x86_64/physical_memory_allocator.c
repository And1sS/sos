#include "../physical_memory_allocator.h"

#define IS_INSIDE(start_1, end_1, start_2, end_2)                              \
    (((start_1) >= (start_2)) && ((end_1) <= (end_2)))

const u32 FRAME_SIZE = 4096;

u64 free_frame = NULL;

memory_map* memory_map_data;
u64 kernel_start, kernel_end;
u64 multiboot_start, multiboot_end;
u64 end_of_memory;

u64 find_end_of_memory() {
    u64 max_block_end = NULL;

    for (u32 i = 0; i < memory_map_data->entries_count; ++i) {
        memory_map_entry_v0* mm_entry = &memory_map_data->entries[i];
        if (mm_entry->type != 1)
            continue;

        u64 block_end = mm_entry->base_addr + mm_entry->length;
        if (max_block_end < block_end)
            max_block_end = block_end;
    }

    return max_block_end;
}

void init_physical_allocator(multiboot_info* multiboot_info) {
    kernel_start = 0xFFFFFFFFFFFFFFFF;
    kernel_end = 0;

    for (u32 i = 1; i < multiboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &multiboot_info->elf_sections.sections[i];
        if (section->addr < kernel_start)
            kernel_start = section->addr;
        if (section->addr + section->size > kernel_end)
            kernel_end = section->addr + section->size;
    }

    memory_map_data = &multiboot_info->mmap;

    multiboot_start = (u64) multiboot_start;
    multiboot_end = multiboot_start + multiboot_info->size;

    end_of_memory = find_end_of_memory();
}

void* allocate_frame() {
    while (free_frame < end_of_memory) {
        for (u32 i = 0; i < memory_map_data->entries_count; ++i) {
            memory_map_entry_v0* mm_entry = &memory_map_data->entries[i];

            if (mm_entry->type != 1
                || !IS_INSIDE(free_frame, free_frame + FRAME_SIZE,
                              mm_entry->base_addr,
                              mm_entry->base_addr + mm_entry->length)
                || IS_INSIDE(free_frame, free_frame + FRAME_SIZE, kernel_start,
                             kernel_end)
                || IS_INSIDE(free_frame, free_frame + FRAME_SIZE,
                             multiboot_start, multiboot_end))
                continue;

            free_frame += FRAME_SIZE;
            return (void*) free_frame;
        }

        free_frame += FRAME_SIZE;
    }

    return NULL;
}
