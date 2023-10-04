#include "pmm.h"

paddr last_available_frame = NULL;
u64 available = 0;

paddr allocate_frame() {
    // TODO: add locks
    if (last_available_frame == NULL) {
        return NULL;
    }

    paddr allocated = last_available_frame;
    last_available_frame = *(paddr*) P2V(last_available_frame);
    available--;

    return allocated;
}

void free_frame(paddr frame) {
    // TODO: add locks
    *(paddr*) P2V(frame) = last_available_frame;
    last_available_frame = frame;
    available++;
}

u64 get_available_frames_count() { return available; }