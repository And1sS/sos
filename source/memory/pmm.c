#include "pmm.h"

paddr last_available_frame = NULL;

#include "../util.h"

void init_pmm(const multiboot_info* const multiboot_info) {
    UNUSED(multiboot_info);
}

paddr allocate_frame() {
    // TODO: add locks
    paddr allocated = last_available_frame;
    last_available_frame = *(paddr*) P2V(last_available_frame);
    return allocated;
}

void deallocate_frame(paddr frame) {
    // TODO: add locks
    *(paddr*) P2V(frame) = last_available_frame;
    last_available_frame = frame;
}