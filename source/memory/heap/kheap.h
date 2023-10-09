#ifndef SOS_KHEAP_H
#define SOS_KHEAP_H

#include "../../lib/types.h"

#define HEAP_MAX_SIZE 0x10000000000 // 1TB
#define HEAP_INITIAL_SIZE 0xA00000  // 10MB

void* kmalloc(u64 size);
void kfree(void* addr);

#endif // SOS_KHEAP_H
