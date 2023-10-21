#include "memory_init.h"
#include "../../memory/heap/kheap.h"
#include "../../memory/pmm.h"

#include "identity_map.h"

void init_kheap();

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

    //1-, 2-, 3-, gap(1872), 4-, 5+
    //1+, 2-, 3-, gap(1872), 4-, 5+
    //1+, 2-, gap(2904), 4-, 5+
    //gap(4072), 4-, 5+
    //1+, 2-, gap(2904), 4-
    void* test1 = kmalloc(100);
    void* test2 = kmalloc(1000);
    void* test3 = kmalloc(1000);
    void* test4 = kmalloc_aligned(10000, FRAME_SIZE);

    kfree(test1);
    kfree(test3);
    kfree(test2);
    kfree(test4);

    for (int i = 0; i < 10000; ++i) {
        kmalloc(1000);
    }

    println("here!");
}
