#include "../../../interrupts/interrupts.h"
#include "../cpu/rflags.h"

bool interrupts_enabled() {
    u64 rflags;

    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=rm"(rflags)
                     :
                     : "memory");

    return rflags & RFLAGS_IRQ_ENABLED_FLAG ? true : false;
}

void enable_interrupts() { __asm__ volatile("sti"); }

void disable_interrupts() { __asm__ volatile("cli"); }