#include "../physical_memory_allocator.h"

#define IS_INSIDE(start_1, end_1, start_2, end_2)                              \
    ((start_1) >= (start_2)) && ((end_1) <= (end_2))

const u32 FRAME_SIZE = 4096;

u64 free_frame = NULL;

parsed_memory_map* _memory_map;
u64 _kernel_start, _kernel_end;
u64 _multiboot_start, _multiboot_end;
u64 last_memory_block_end;

u64 find_end_of_memory() {
    u64 max_block_end = NULL;

    for (int i = 0; i < _memory_map->entries_count; ++i) {
        parsed_memory_map_entry_v0* mm_entry = &_memory_map->entries[i];
        if (mm_entry->type != 1)
            continue;

        u64 block_end = mm_entry->base_addr + mm_entry->length;
        if (max_block_end < block_end)
            max_block_end = block_end;
    }

    return max_block_end;
}

void init_physical_allocator(parsed_memory_map* memory_map, u64 kernel_start,
                             u64 kernel_end, u64 multiboot_start,
                             u64 multiboot_end) {

    _memory_map = memory_map;
    _kernel_start = kernel_start, _kernel_end = kernel_end;
    _multiboot_start = multiboot_start, _multiboot_end = multiboot_end;
    last_memory_block_end = find_end_of_memory();
}

void* allocate_frame() {
    while (free_frame < last_memory_block_end) {
        for (int i = 0; i < _memory_map->entries_count; ++i) {
            parsed_memory_map_entry_v0* mm_entry = &_memory_map->entries[i];

            if (mm_entry->type != 1
                || !IS_INSIDE(free_frame, free_frame + FRAME_SIZE,
                              mm_entry->base_addr,
                              mm_entry->base_addr + mm_entry->length)
                || IS_INSIDE(free_frame, free_frame + FRAME_SIZE, _kernel_start,
                             _kernel_end)
                || IS_INSIDE(free_frame, free_frame + FRAME_SIZE,
                             _multiboot_start, _multiboot_end))
                continue;

            free_frame += FRAME_SIZE;
            return (void*) free_frame;
        }

        free_frame += FRAME_SIZE;
    }

    return NULL;
}
