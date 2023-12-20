#include "../../../lib/util.h"

#define SCHEDULER_ISR_NUMBER 250 // TODO: think of gentrifying isr handlers

void schedule() {
    __asm__ volatile("int $" STR(SCHEDULER_ISR_NUMBER) : : : "memory");
}
