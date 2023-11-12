#include "pmm.h"
#include "../lib/memory_util.h"
#include "../spin_lock.h"

lock pmm_lock;

volatile paddr last_available_frame = NULL;
volatile u64 available = 0;

void pmm_init() { init_lock(&pmm_lock); }

paddr allocate_frame() {
    bool interrupts_enabled = spin_lock_irq_save(&pmm_lock);
    if (last_available_frame == NULL) {
        spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);
        return NULL;
    }

    paddr allocated = last_available_frame;
    last_available_frame = *(paddr*) P2V(last_available_frame);
    available--;

    spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);
    return allocated;
}

paddr allocate_zeroed_frame() {
    paddr frame = allocate_frame();
    if (frame == NULL) {
        return NULL;
    }

    memset((void*) P2V(frame), 0, FRAME_SIZE);
    return frame;
}

void free_frame(paddr frame) {
    bool interrupts_enabled = spin_lock_irq_save(&pmm_lock);
    *(paddr*) P2V(frame) = last_available_frame;
    last_available_frame = frame;
    available++;
    spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);
}

u64 get_available_frames_count() {
    bool interrupts_enabled = spin_lock_irq_save(&pmm_lock);
    u64 result = available;
    spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);

    return result;
}