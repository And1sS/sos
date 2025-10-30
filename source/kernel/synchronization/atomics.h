#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/types.h"

// has read-acquire semantics
extern u64 atomic_exchange(volatile u64* addr, u64 new_value);

// has write-release semantics
extern void atomic_set(volatile u64* addr, u64 value);

// has read-acquire semantics
extern u64 atomic_get(volatile u64* addr);

// has write-release semantics
extern void atomic_or(volatile u64* addr, u64 mask);

extern void atomic_increment(volatile u64* addr);

extern u64 atomic_increment_and_get(volatile u64* addr);

extern void atomic_decrement(volatile u64* addr);

extern u64 atomic_decrement_and_get(volatile u64* addr);

#endif