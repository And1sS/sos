#include "../../../interrupts/irq.h"
#include "../cpu/rflags.h"

bool local_irq_enabled() {
    return get_rflags() & RFLAGS_IRQ_ENABLED_FLAG ? true : false;
}

void local_irq_enable() { __asm__ volatile("sti"); }

void local_irq_disable() { __asm__ volatile("cli"); }