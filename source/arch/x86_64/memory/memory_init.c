#include "memory_init.h"
#include "../../../memory/heap/kheap.h"
#include "../../../memory/pmm.h"

#include "identity_map.h"

void init_memory(const multiboot_info* const mboot_info) {
    init_pmm();
    identity_map_ram(mboot_info);
    print("Finished memory mapping! Free frames: ");
    print_u64(get_available_frames_count());
    println("");

    kmalloc_init();
    print("Finished kernel heap initialization! Heap initial size: ");
    print_u64(KHEAP_INITIAL_SIZE);
    println("");
}
