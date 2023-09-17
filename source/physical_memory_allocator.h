#ifndef SOS_PHYSICAL_MEMORY_MANAGER_H
#define SOS_PHYSICAL_MEMORY_MANAGER_H

#include "multiboot.h"
#include "types.h"

extern const u32 FRAME_SIZE;

void init_physical_allocator(parsed_multiboot_info* multiboot_info);

void* allocate_frame();
void deallocate_frame();

#endif // SOS_PHYSICAL_MEMORY_MANAGER_H
