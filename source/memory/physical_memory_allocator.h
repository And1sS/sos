#ifndef SOS_PHYSICAL_MEMORY_MANAGER_H
#define SOS_PHYSICAL_MEMORY_MANAGER_H

#include "../lib/types.h"
#include "../multiboot.h"

extern const u32 FRAME_SIZE;

void init_physical_allocator(const multiboot_info* const multiboot_info);

void* allocate_frame();
bool deallocate_frame(u64 frame);

#endif // SOS_PHYSICAL_MEMORY_MANAGER_H
