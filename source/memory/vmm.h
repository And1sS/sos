#ifndef SOS_VIRTUAL_MEMORY_MANAGER_H
#define SOS_VIRTUAL_MEMORY_MANAGER_H

#include "memory_map.h"

u64 get_page(vaddr virtual_page);
bool map_page(vaddr virtual_page, paddr physical_page, u16 flags);

#endif // SOS_VIRTUAL_MEMORY_MANAGER_H
