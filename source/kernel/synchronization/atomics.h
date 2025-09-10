#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/types.h"

extern u64 atomic_exchange(volatile u64* addr, volatile u64 new_value);

extern void atomic_set(volatile u64* addr, volatile u64 value);

extern void atomic_increment(volatile u64* addr);

#endif