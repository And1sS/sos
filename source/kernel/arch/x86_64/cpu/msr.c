#include "msr.h"

u64 msr_read(u32 reg) {
    u32 low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(reg) : "memory");
    return ((u64) high << 32) | low;
}

void msr_write(u32 msr, u64 value) {
    u32 low = value & 0xFFFFFFFF;
    u32 high = (value >> 32) & 0xFFFFFFFF;
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}