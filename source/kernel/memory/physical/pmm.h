#ifndef SOS_PHYSICAL_MEMORY_MANAGER_H
#define SOS_PHYSICAL_MEMORY_MANAGER_H

#include "../memory_map.h"

paddr pmm_allocate_frame();
paddr pmm_allocate_zeroed_frame();
void pmm_free_frame(paddr frame);
u64 pmm_frames_available();

#endif // SOS_PHYSICAL_MEMORY_MANAGER_H
