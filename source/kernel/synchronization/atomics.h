#ifndef SOS_ATOMICS_H
#define SOS_ATOMICS_H

#include "../lib/memory_util.h"
#include "../lib/types.h"
#include "barriers.h"

/*
 * These macros provide atomic, non-tearing, single-copy load/store
 * semantics for plain C variables — without introducing memory barriers.
 */

#define __READ_ONCE_SIZE(p, res, size)                                         \
    do {                                                                       \
        switch (size) {                                                        \
        case 1:                                                                \
            *(u8*) (res) = *(volatile u8*) (p);                                \
            break;                                                             \
        case 2:                                                                \
            *(u16*) (res) = *(volatile u16*) (p);                              \
            break;                                                             \
        case 4:                                                                \
            *(u32*) (res) = *(volatile u32*) (p);                              \
            break;                                                             \
        case 8:                                                                \
            *(u64*) (res) = *(volatile u64*) (p);                              \
            break;                                                             \
        default:                                                               \
            barrier();                                                         \
            __builtin_memcpy((res), (const void*) (p), size);                  \
            barrier();                                                         \
            break;                                                             \
        }                                                                      \
    } while (0)

#define __WRITE_ONCE_SIZE(p, res, size)                                        \
    do {                                                                       \
        switch (size) {                                                        \
        case 1:                                                                \
            *(volatile u8*) (p) = *(u8*) (res);                                \
            break;                                                             \
        case 2:                                                                \
            *(volatile u16*) (p) = *(u16*) (res);                              \
            break;                                                             \
        case 4:                                                                \
            *(volatile u32*) (p) = *(u32*) (res);                              \
            break;                                                             \
        case 8:                                                                \
            *(volatile u64*) (p) = *(u64*) (res);                              \
            break;                                                             \
        default:                                                               \
            barrier();                                                         \
            __builtin_memcpy((void*) (p), (res), size);                        \
            barrier();                                                         \
            break;                                                             \
        }                                                                      \
    } while (0)

#define READ_ONCE(x)                                                           \
    ({                                                                         \
        union {                                                                \
            typeof(x) __val;                                                   \
            char __c[1];                                                       \
        } __u = {.__c = {0}};                                                  \
        __READ_ONCE_SIZE(&(x), __u.__c, sizeof(x));                            \
        __u.__val;                                                             \
    })

#define WRITE_ONCE(x, val)                                                     \
    ({                                                                         \
        union {                                                                \
            typeof(x) __val;                                                   \
            char __c[1];                                                       \
        } __u = {.__val = (val)};                                              \
        __WRITE_ONCE_SIZE(&(x), __u.__c, sizeof(x));                           \
        __u.__val;                                                             \
    })

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