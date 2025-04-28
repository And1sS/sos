#include "hash.h"

// Cantor algorithm
u64 hash2(u64 a, u64 b) {
    return (a + b + 1) * (a + b) / 2 + b;
}

u64 hash3(u64 a, u64 b, u64 c) {
    return hash2(a, hash2(b, c));
}