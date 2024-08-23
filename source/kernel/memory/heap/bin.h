#ifndef SOS_KHEAP_BIN_H
#define SOS_KHEAP_BIN_H

#include "block.h"

typedef struct {
    block* first;

#ifdef KHEAP_DEBUG
    u64 blocks;
#endif
} bin;

void bin_insert(bin* b, block* blk);
block* bin_remove(bin* b, block* blk);
block* bin_minimal_fit_block(bin* b, u64 size);
void bin_print(bin* b);

#endif // SOS_KHEAP_BIN_H
