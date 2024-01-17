#ifndef SOS_KHEAP_H
#define SOS_KHEAP_H

#include "../../lib/types.h"

#define KHEAP_MAX_SIZE 0x10000000000 // 1TB
#define KHEAP_INITIAL_SIZE 0x100000  // 1MB

void kheap_init();
void* kmalloc(u64 size);
// alignment must be multiple of 8
void* kmalloc_aligned(u64 size, u64 alignment);
void* krealloc(void* addr, u64 size);
void kfree(void* addr);
u64 kheap_size();

// used for debug
void kheap_print();

#endif // SOS_KHEAP_H
