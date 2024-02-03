#include "pmm.h"
#include "../../arch/common/vmm.h"
#include "../../boot/multiboot.h"
#include "../../lib/memory_util.h"
#include "../../synchronization/spin_lock.h"

lock pmm_lock = SPIN_LOCK_STATIC_INITIALIZER;

volatile paddr last_available_frame = NULL;
volatile u64 available = 0;

paddr pmm_allocate_frame() {
    bool interrupts_enabled = spin_lock_irq_save(&pmm_lock);
    if (!last_available_frame) {
        spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);
        return NULL;
    }

    paddr allocated = last_available_frame;
    last_available_frame = *(paddr*) P2V(last_available_frame);
    available--;

    spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);
    return allocated;
}

paddr pmm_allocate_zeroed_frame() {
    paddr frame = pmm_allocate_frame();
    if (!frame)
        return NULL;

    memset((void*) P2V(frame), 0, PAGE_SIZE);
    return frame;
}

void pmm_free_frame(paddr frame) {
    frame &= ~(PAGE_SIZE - 1);
    bool interrupts_enabled = spin_lock_irq_save(&pmm_lock);
    *(paddr*) P2V(frame) = last_available_frame;
    last_available_frame = frame;
    available++;
    spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);
}

u64 pmm_frames_available() {
    bool interrupts_enabled = spin_lock_irq_save(&pmm_lock);
    u64 result = available;
    spin_unlock_irq_restore(&pmm_lock, interrupts_enabled);

    return result;
}