#ifndef SOS_PHYSICAL_MEMORY_MANAGER_H
#define SOS_PHYSICAL_MEMORY_MANAGER_H

#include "multiboot.h"
#include "types.h"

extern const u32 FRAME_SIZE;

void init_physical_allocator(parsed_memory_map* memory_map, u64 kernel_start,
                             u64 kernel_end, u64 multiboot_start,
                             u64 multiboot_end);

void* allocate_frame();
void deallocate_frame();

#endif // SOS_PHYSICAL_MEMORY_MANAGER_H
