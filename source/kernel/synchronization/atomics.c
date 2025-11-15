#include "atomics.h"

bool atomic_increment_not_zero(volatile u64* addr) {
    u64 old, value = atomic_get(addr);

    while (true) {
        if (!value)
            return false;

        old = atomic_compare_exchange(addr, value, value + 1);
        if (old == value)
            return true;

        value = old;
    }
}

bool atomic_decrement_not_one(volatile u64* addr) {
    u64 old, value = atomic_get(addr);

    while (true) {
        if (value == 1)
            return false;

        old = atomic_compare_exchange(addr, value, value - 1);
        if (value == old)
            return true;

        value = old;
    }
}
