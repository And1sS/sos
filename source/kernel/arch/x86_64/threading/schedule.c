#include "../../../lib/util.h"

#define SCHEDULE_ISR_NUMBER 250
#define SCHEDULE_THREAD_EXIT_ISR_NUMBER 251

void schedule() {
    __asm__ volatile("int $" STR(SCHEDULE_ISR_NUMBER) : : : "memory");
}

void schedule_thread_exit() {
    __asm__ volatile("int $" STR(SCHEDULE_THREAD_EXIT_ISR_NUMBER)
                     :
                     :
                     : "memory");
}
