#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/memory_util.h"
#include "../lib/types.h"
#include "barriers.h"

// These macros provide atomic, single-copy load/store semantics
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#define READ_ONCE(x) \
({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })

#define WRITE_ONCE(x, val) \
do { ACCESS_ONCE(x) = (val); } while (0)

// has read-acquire semantics
extern u64 atomic_exchange(volatile u64* addr, u64 new_value);

// returns old value
extern u64 atomic_compare_exchange(volatile u64* addr, u64 old_value,
                                   u64 new_value);

// has write-release semantics
extern void atomic_set(volatile u64* addr, u64 value);

// has read-acquire semantics
extern u64 atomic_get(volatile u64* addr);

// has write-release semantics
extern void atomic_or(volatile u64* addr, u64 mask);

// has write-release semantics
extern void atomic_and(volatile u64* addr, u64 mask);

extern void atomic_increment(volatile u64* addr);

// returns true when atomically increments any value > 0,
// or false when observed value was 0
bool atomic_increment_not_zero(volatile u64* addr);

extern u64 atomic_increment_and_get(volatile u64* addr);

extern void atomic_decrement(volatile u64* addr);

bool atomic_decrement_greater_than(volatile u64* addr, u64 threshold);

// returns true when atomically decrements any value > 1,
// or false when observed value was 1
bool atomic_decrement_not_one(volatile u64* addr);

extern u64 atomic_decrement_and_get(volatile u64* addr);

#endif