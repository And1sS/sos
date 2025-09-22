#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/types.h"

// has read-acquire semantics
extern u64 atomic_exchange(volatile u64* addr, volatile u64 new_value);

// has write-release semantics
extern void atomic_set(volatile u64* addr, volatile u64 value);

extern void atomic_increment(volatile u64* addr);

extern u64 atomic_decrement_and_get(volatile u64* addr);

#endif