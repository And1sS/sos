#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/types.h"

// These macros provide atomic, single-copy load/store semantics
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#define READ_ONCE(x) \
({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })

#define WRITE_ONCE(x, val) \
do { ACCESS_ONCE(x) = (val); } while (0)

// has read-acquire semantics
extern u64 atomic_exchange(volatile u64* addr, volatile u64 new_value);

// has write-release semantics
extern void atomic_set(volatile u64* addr, volatile u64 value);

extern void atomic_increment(volatile u64* addr);

extern u64 atomic_decrement_and_get(volatile u64* addr);

#endif