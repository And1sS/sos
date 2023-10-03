#ifndef SOS_MEMORY_H
#define SOS_MEMORY_H

#include "../lib/types.h"

#define paddr u64
#define vaddr u64

#define IS_CANONICAL(vaddr)                                                    \
    ((vaddr) < 0x0000800000000000) || ((vaddr) > 0XFFFF7FFFFFFFFFFF)

#define FRAME_SIZE 4096;
#define RESERVED_LOWER_PMEM_SIZE 1024 * 1024; // 1MB

#define KERNEL_START_VADDR 0xFFFF800000000000       // 256 entry in p4
#define KERNEL_VMAPPED_RAM_START_VADDR 0XFFFF888000000000 // 273 entry in p4

#define P2V(addr) ((addr) + KERNEL_VMAPPED_RAM_START_VADDR)
#define V2P(addr) ((addr) -KERNEL_VMAPPED_RAM_START_VADDR)

#endif // SOS_MEMORY_H
