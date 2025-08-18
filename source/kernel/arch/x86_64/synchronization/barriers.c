#include "../../../synchronization/barriers.h"

void smp_mb() { asm volatile("mfence" ::: "memory"); }

void smp_wb() { asm volatile("sfence" ::: "memory"); }

void smp_rb() { asm volatile("lfence" ::: "memory"); }
