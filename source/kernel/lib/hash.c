#include "hash.h"

u64 hash(u64 a) {
    a ^= a >> 33;
    a *= 0xff51afd7ed558ccdL;
    a ^= a >> 33;
    a *= 0xc4ceb9fe1a85ec53L;
    a ^= a >> 33;
    return a;
}

// Cantor algorithm
u64 hash2(u64 a, u64 b) {
    return (a + b + 1) * (a + b) / 2 + b;
}

u64 hash3(u64 a, u64 b, u64 c) {
    return hash2(a, hash2(b, c));
}