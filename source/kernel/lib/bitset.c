#include "bitset.h"

bitset* bitset_create() {
    bitset* result = (bitset*) kmalloc(sizeof(bitset));
    if (!result) {
        return NULL;
    }

    array_list* chunks =
        array_list_create(BITSET_INITIAL_CHUNKS * sizeof(chunk));
    if (!chunks) {
        return NULL;
    }

    array_list_clear(chunks);

    result->chunks = chunks;
    return result;
}

u64 bitset_allocate_index(bitset* set) {
    for (u64 i = 0; i < set->chunks->size; ++i) {
        chunk ch = (chunk) array_list_get(set->chunks, i);
        chunk inverted = ~ch;

        if (inverted > 0) {
            i8 empty_idx = lsb_u64(inverted);

            if (empty_idx >= 0) {
                chunk updated = ch | ((u64) 1 << empty_idx);
                array_list_set(set->chunks, i, (void*) updated);
                return i * BITS_IN_CHUNK + empty_idx;
            }
        }
    }

    u64 bits = set->chunks->size * BITS_IN_CHUNK;
    array_list_add_last(set->chunks, (void*) 1);
    return bits;
}

bool bitset_free_index(bitset* set, u64 index) {
    u64 chunk_index = index / BITS_IN_CHUNK;
    u64 bit_index = index % BITS_IN_CHUNK;

    if (chunk_index >= set->chunks->size) {
        return false;
    }

    chunk ch = (chunk) array_list_get(set->chunks, chunk_index);
    if (!((ch >> bit_index) & 1)) {
        return false;
    }

    chunk updated = ch & ~((u64) 1 << bit_index);
    array_list_set(set->chunks, chunk_index, (void*) updated);
    return true;
}
