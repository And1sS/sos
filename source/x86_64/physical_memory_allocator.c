#include "../memory/physical_memory_allocator.h"
#include "../lib/alignment.h"
#include "../lib/array_stack.h"
#include "../lib/math.h"

#define IS_INSIDE(start_1, end_1, start_2, end_2)                              \
    (((start_1) >= (start_2)) && ((end_1) <= (end_2)))

#define INTERSECTS(start_1, end_1, start_2, end_2)                             \
    IS_INSIDE(start_1, end_1, start_2, end_2)                                  \
    || (((start_1) <= (start_2)) && ((start_2) <= (end_1)))                    \
        || (((start_1) <= (end_2)) && ((end_2) <= (end_1)))

#define FITS(size, start, end)                                                 \
    ((size > 0) && ((end) > (start)) && ((end) - (start) >= (size) -1))

#define SWAP(a, b)                                                             \
    {                                                                          \
        typeof(a) c = a;                                                       \
        a = b;                                                                 \
        b = c;                                                                 \
    }

const u32 FRAME_SIZE = 4096;
const u64 RESERVED_LOWER_MEM_SIZE = 1024 * 1024; // 1MB

array_stack unused_frames;

// all ends are inclusive
u64 kernel_start, kernel_end;
u64 multiboot_start, multiboot_end;
u64 stack_array_start, stack_array_end;

u64 end_of_memory;

u64 find_end_of_memory(const memory_map* const memory_map_data);

u64 find_kernel_start(const elf_sections* const elf_sections);
u64 find_kernel_end(const elf_sections* const elf_sections);

u64 allocate_memory_for_stack(const memory_map* const memory_map_data,
                              u64 kernel_start, u64 kernel_end,
                              u64 multiboot_start, u64 multiboot_end, u64 size);
void init_frames_stack(const memory_map* const memory_map_data,
                       u64 kernel_start, u64 kernel_end, u64 multiboot_start,
                       u64 multiboot_end);

void init_physical_allocator(const multiboot_info* const multiboot_info) {
    kernel_start = find_kernel_start(&multiboot_info->elf_sections);
    kernel_end = find_kernel_end(&multiboot_info->elf_sections);

    multiboot_start = (u64) multiboot_info->original_struct_addr;
    multiboot_end = multiboot_start + multiboot_info->size - 1;

    end_of_memory = find_end_of_memory(&multiboot_info->mmap);
    init_frames_stack(&multiboot_info->mmap, kernel_start, kernel_end,
                      multiboot_start, multiboot_end);
}

void* allocate_frame() {
    // TODO: add locks
    return can_pop(&unused_frames) ? (void*) pop(&unused_frames) : NULL;
}

bool deallocate_frame(u64 frame) {
    // TODO: add locks
    return push(&unused_frames, frame);
}

u64 find_kernel_start(const elf_sections* const elf_sections) {
    u64 kernel_start = 0xFFFFFFFFFFFFFFFF;

    for (u32 i = 1; i < elf_sections->sections_number; ++i) {
        elf64_shdr* section = &elf_sections->sections[i];
        if (section->addr < kernel_start)
            kernel_start = section->addr;
    }

    return kernel_start;
}

u64 find_kernel_end(const elf_sections* const elf_sections) {
    u64 kernel_end = 0;

    for (u32 i = 1; i < elf_sections->sections_number; ++i) {
        elf64_shdr* section = &elf_sections->sections[i];
        u64 section_end = section->addr + section->size - 1;

        if (section_end > kernel_end)
            kernel_end = section_end;
    }

    return kernel_end;
}

u64 find_end_of_memory(const memory_map* const memory_map_data) {
    u64 max_block_end = NULL;

    for (u32 i = 0; i < memory_map_data->entries_count; ++i) {
        memory_map_entry_v0* mm_entry = &memory_map_data->entries[i];
        if (mm_entry->type != 1)
            continue;

        u64 block_end = mm_entry->base_addr + mm_entry->length - 1;
        if (max_block_end < block_end)
            max_block_end = block_end;
    }

    return max_block_end;
}

#include "../vga_print.h"
void push_all_available_frames(const memory_map* const memory_map_data,
                               u64 kernel_start, u64 kernel_end,
                               u64 multiboot_start, u64 multiboot_end,
                               u64 stack_array_start, u64 stack_array_end) {

    for (u64 frame = RESERVED_LOWER_MEM_SIZE; frame < end_of_memory;
         frame += FRAME_SIZE) {
        for (u32 i = 0; i < memory_map_data->entries_count; i++) {
            memory_map_entry_v0* mm_entry = &memory_map_data->entries[i];

            if (!IS_INSIDE(frame, frame + FRAME_SIZE, mm_entry->base_addr,
                           mm_entry->base_addr + mm_entry->length - 1))
                continue;

            if (mm_entry->type != 1)
                continue;

            if (IS_INSIDE(frame, frame + FRAME_SIZE, kernel_start, kernel_end)
                || IS_INSIDE(frame, frame + FRAME_SIZE, multiboot_start,
                             multiboot_end)
                || IS_INSIDE(frame, frame + FRAME_SIZE, stack_array_start,
                             stack_array_end))
                continue;

            // TODO: add locks
            push(&unused_frames, frame);
        }
    }
}

void init_frames_stack(const memory_map* const memory_map_data,
                       u64 kernel_start, u64 kernel_end, u64 multiboot_start,
                       u64 multiboot_end) {

    u64 stack_entries = align_to_upper(end_of_memory, FRAME_SIZE) / FRAME_SIZE;
    u64 stack_size = align_to_upper(stack_entries * sizeof(u64), FRAME_SIZE);
    u64 stack_data =
        allocate_memory_for_stack(memory_map_data, kernel_start, kernel_end,
                                  multiboot_start, multiboot_end, stack_size);
    // TODO: map all pages for stack to virtual memory

    stack_array_start = stack_data;
    stack_array_end = stack_data + stack_size - 1;

    init_stack(&unused_frames, stack_entries, (u64*) stack_data);
    println("");
    print("allocated physical memory frames stack s: ");
    print_u64_hex(stack_data);
    println("");
    print("stack size: ");
    print_u64(stack_size);
    //    push_all_available_frames(memory_map_data, kernel_start, kernel_end,
    //                              multiboot_start, multiboot_end,
    //                              stack_array_start, stack_array_end);
}

u64 try_allocate_with_gap(u64 desired_size, u64 segment_start, u64 segment_end,
                          u64 gap_start, u64 gap_end) {

    gap_start = MAX(gap_start, segment_start);
    gap_end = MIN(gap_end, segment_end);

    u64 below_gap_segment_start = align_to_upper(segment_start, FRAME_SIZE);
    if (FITS(desired_size, below_gap_segment_start, gap_start))
        return below_gap_segment_start;

    u64 above_gap_segment_start = align_to_upper(gap_end + 1, FRAME_SIZE);
    if (FITS(desired_size, above_gap_segment_start, segment_end))
        return above_gap_segment_start;

    return NULL;
}

u64 try_allocate_with_gaps(u64 desired_size, u64 segment_start, u64 segment_end,
                           u64 gap_1_start, u64 gap_1_end, u64 gap_2_start,
                           u64 gap_2_end) {

    gap_1_start = MAX(segment_start, gap_1_start);
    gap_1_end = MIN(gap_1_end, segment_end);
    gap_2_start = MAX(segment_start, gap_2_start);
    gap_2_end = MIN(segment_end, gap_2_end);

    if (gap_1_start > gap_2_start) {
        SWAP(gap_1_start, gap_2_start);
        SWAP(gap_1_end, gap_2_end);
    }

    u64 aligned_seg_start = align_to_upper(segment_start, FRAME_SIZE);
    if (FITS(desired_size, aligned_seg_start, gap_1_start - 1))
        return aligned_seg_start;

    u64 aligned_gap_1_end = align_to_upper(gap_1_end + 1, FRAME_SIZE);
    if (FITS(desired_size, aligned_gap_1_end, gap_2_start - 1))
        return aligned_gap_1_end;

    u64 aligned_gap_2_end = align_to_upper(gap_2_end + 1, FRAME_SIZE);
    if (FITS(desired_size, aligned_gap_2_end, segment_end - 1))
        return aligned_gap_2_end;

    return NULL;
}

u64 try_allocate_memory_for_stack(memory_map_entry_v0 segment, u64 kernel_start,
                                  u64 kernel_end, u64 multiboot_start,
                                  u64 multiboot_end, u64 size) {

    if (segment.type != 1)
        return NULL;

    u64 seg_start = segment.base_addr;
    u64 seg_end = seg_start + segment.length;
    u64 aligned_seg_start = align_to_upper(seg_start, FRAME_SIZE);

    if (seg_end < RESERVED_LOWER_MEM_SIZE)
        return NULL;

    bool contains_kernel =
        INTERSECTS(kernel_start, kernel_end, seg_start, seg_end);
    bool contains_multiboot =
        INTERSECTS(multiboot_start, multiboot_end, seg_start, seg_end);

    if (!contains_kernel && !contains_multiboot
        && FITS(size, aligned_seg_start, seg_end)) {
        return aligned_seg_start;
    } else if (contains_kernel && !contains_multiboot) {
        return try_allocate_with_gap(size, seg_start, seg_end, kernel_start,
                                     kernel_end);
    } else if (contains_multiboot && !contains_kernel) {
        return try_allocate_with_gap(size, seg_start, seg_end, kernel_start,
                                     kernel_end);
    } else if (contains_multiboot && contains_kernel) {
        return try_allocate_with_gaps(size, seg_start, seg_end, kernel_start,
                                      kernel_end, multiboot_start,
                                      multiboot_end);
    }

    return NULL;
}

u64 allocate_memory_for_stack(const memory_map* const memory_map_data,
                              u64 kernel_start, u64 kernel_end,
                              u64 multiboot_start, u64 multiboot_end,
                              u64 size) {

    for (u32 i = 1; i < memory_map_data->entries_count; i++) {
        u64 result = try_allocate_memory_for_stack(
            memory_map_data->entries[i], kernel_start, kernel_end,
            multiboot_start, multiboot_end, size);

        if (result != NULL)
            return result;
    }

    return NULL;
}