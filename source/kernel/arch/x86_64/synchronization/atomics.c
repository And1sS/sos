#include "../../../synchronization/atomics.h"

u64 atomic_exchange(volatile u64* addr, volatile u64 new_value) {
    u64 old_value;

    __asm__ volatile("lock xchgq %0, %1"
                     : "=r"(old_value), "+m"(*addr)
                     : "0"(new_value)
                     : "memory");

    return old_value;
}

void atomic_set(volatile u64* addr, volatile u64 value) {
    *addr = value;

    __asm__ volatile("mfence" : : : "memory");
}

void atomic_increment(volatile u64* addr) {
    __asm__ volatile("lock incq (%0)" : : "r"(addr) : "memory");
}