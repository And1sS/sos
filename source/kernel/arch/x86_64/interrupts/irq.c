#include "../../../interrupts/irq.h"
#include "../cpu/rflags.h"

bool local_irq_enabled() {
    return get_rflags() & RFLAGS_IRQ_ENABLED_FLAG ? true : false;
}

void local_irq_enable() { __asm__ volatile("sti"); }

void local_irq_disable() { __asm__ volatile("cli"); }

bool local_irq_save() {
    bool interrupts_enabled = local_irq_enabled();
    local_irq_disable();
    return interrupts_enabled;
}

void local_irq_restore(bool interrupts_enabled) {
    if (interrupts_enabled) {
        local_irq_enable();
    }
}