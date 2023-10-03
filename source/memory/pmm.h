#ifndef SOS_PHYSICAL_MEMORY_MANAGER_H
#define SOS_PHYSICAL_MEMORY_MANAGER_H

#include "../multiboot.h"
#include "memory.h"

void init_pmm(const multiboot_info* const multiboot_info);

paddr allocate_frame();
void free_frame(paddr frame);

#endif // SOS_PHYSICAL_MEMORY_MANAGER_H
