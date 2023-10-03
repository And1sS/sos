#include "pmm.h"

paddr last_available_frame = NULL;

paddr allocate_frame() {
    // TODO: add locks
    paddr allocated = last_available_frame;
    last_available_frame = *(paddr*) P2V(last_available_frame);
    return allocated;
}

void free_frame(paddr frame) {
    // TODO: add locks
    *(paddr*) P2V(frame) = last_available_frame;
    last_available_frame = frame;
}