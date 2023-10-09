#include "kheap.h"
#include "../../memory/memory_map.h"

u64 last_addr = KHEAP_START_VADDR;

// Just placeholders for now
void* kmalloc(u64 size) {
    void* reserved = (void*) last_addr;
    last_addr += size;
    return reserved;
}

#include "../../util.h"
void kfree(void* addr) { UNUSED(addr); }