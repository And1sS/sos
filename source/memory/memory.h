#ifndef SOS_MEMORY_H
#define SOS_MEMORY_H

#include "../lib/types.h"
#include "../multiboot.h"
#include "memory_map.h"
#include "pmm.h"
#include "vmm.h"

void init_memory(const multiboot_info* const mboot_info);

#endif // SOS_MEMORY_H
