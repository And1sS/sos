#ifndef SOS_PHYSICAL_MEMORY_MANAGER_H
#define SOS_PHYSICAL_MEMORY_MANAGER_H

#include "memory_map.h"

void pmm_init();
paddr allocate_frame();
paddr allocate_zeroed_frame();
void free_frame(paddr frame);
u64 get_available_frames_count();

#endif // SOS_PHYSICAL_MEMORY_MANAGER_H
