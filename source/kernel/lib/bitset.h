#ifndef SOS_BITSET_H
#define SOS_BITSET_H

#include "../memory/heap/kheap.h"
#include "container/array_list/array_list.h"
#include "math.h"

#define BITSET_INITIAL_CHUNKS 8
#define BITS_IN_CHUNK 64

#define chunk u64

// bit = 0 -> unused
// bit = 1 -> in use
typedef struct {
    array_list* chunks;
} bitset;

bitset* bitset_create();

bool bitset_allocate_index(bitset* set, u64* result);
bool bitset_free_index(bitset* set, u64 index);

#endif // SOS_BITSET_H
