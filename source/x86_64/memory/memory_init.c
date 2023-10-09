#include "memory_init.h"
#include "../../memory/heap/kheap.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"
#include "identity_map.h"

void init_kheap();

#include "../../lib/memory_util.h"
void init_memory(const multiboot_info* const mboot_info) {
    init_pmm();
    identity_map_ram(mboot_info);
    print("Finished memory mapping! Free frames: ");
    print_u64(get_available_frames_count());
    println("");

    init_kheap();
    print("Finished kernel heap initialization! Heap initial size: ");
    print_u64(HEAP_INITIAL_SIZE);
    println("");
}

void init_kheap() {
    for (u64 vframe = KHEAP_START_VADDR;
         vframe < KHEAP_START_VADDR + HEAP_INITIAL_SIZE; vframe++) {

        u64 page = get_page(vframe);
        if (!(page & 1)) {
            paddr pframe = allocate_frame();
            map_page(vframe, pframe, 1 | 2 | 4);
        }
    }
    memset((void*) KHEAP_START_VADDR, 0, HEAP_INITIAL_SIZE);
}
