#include "cpuid.h"

void cpuid(u32 reg, u32* eax, u32* ebx, u32* ecx, u32* edx) {
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "0"(reg)
                     : "memory");
}
