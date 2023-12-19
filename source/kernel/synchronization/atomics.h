#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/types.h"

u64 atomic_exchange(volatile u64* addr, volatile u64 new_value);

void atomic_set(volatile u64* addr, volatile u64 value);

#endif