#include "../../../lib/util.h"

#define SCHEDULER_ISR_NUMBER 250

void schedule() {
    __asm__ volatile("int $" STR(SCHEDULER_ISR_NUMBER) : : : "memory");
}
