#include "atomics.h"

u64 atomic_exchange(volatile u64* addr, volatile u64 new_value) {
    u32 old_value;

    __asm__ volatile("lock xchg (%2), %1"
                     : "=b"(old_value)
                     : "b"(new_value), "a"(addr));

    return old_value;
}

void atomic_set(volatile u64* addr, volatile u64 value) {
    *addr = value;

    __asm__ volatile ("mfence");
}