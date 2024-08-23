#include "bin.h"
#include "../../lib/kprint.h"
#include "../../lib/panic.h"

void bin_insert(bin* b, block* blk) {
    block_clear_node(blk);

    if (!b->first) {
        b->first = blk;
        return;
    }

    block* first = b->first;
    if (block_size(blk) <= block_size(first)) {
        blk->next = first;
        first->prev = blk;
        b->first = blk;
        return;
    }

    // a->b->(spot)c->null
    // a->b->c->(spot)null
    // cases:
    block* prev = NULL;
    block* cur = first;

    while (cur && block_size(blk) > block_size(cur)) {
        prev = cur;
        cur = cur->next;
    }

    if (prev)
        prev->next = blk;
    else
        b->first = blk;

    blk->next = cur;
    blk->prev = prev;

    if (cur) // a->b->(spot)c->null
        cur->prev = blk;
}

block* bin_remove(bin* b, block* blk) {
    block* prev = blk->prev;
    block* next = blk->next;

    if (!b->first) // empty bin
        panic("Invalid kernel heap bin configuration, size < 0");

    if (prev && next) { // prev <-> X <-> next
        prev->next = next;
        next->prev = prev;
    } else if (!prev && next) { // X <-> next
        b->first = next;
        next->prev = NULL;
    } else if (prev && !next) { // prev <-> X
        prev->next = NULL;
    } else {
        b->first = NULL;
    }

    block_clear_node(blk);
    return blk;
}

void bin_print(bin* b) {
    print("BIN: ");
    block* iter = b->first;
    while (iter) {
        block_print(iter);
        iter = iter->next;
    }
}