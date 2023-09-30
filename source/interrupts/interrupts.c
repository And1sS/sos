#include "interrupts.h"

bool interrupts_enabled() {
    u64 flags;

    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=rm"(flags)
                     :
                     : "memory");

    return (flags >> 9) & 1;
}

void enable_interrupts() { __asm__ volatile("sti"); }

void disable_interrupts() { __asm__ volatile("cli"); }