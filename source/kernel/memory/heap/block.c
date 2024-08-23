#include "block.h"
#include "../../lib/kprint.h"
#include "../../lib/memory_util.h"

footer* block_footer(block* blk) {
    return (footer*) ((u64) blk + block_size(blk) - sizeof(footer));
}

void block_init(block* blk, u64 size, block* prev, block* next) {
    memset(blk, 0, size);

    block_set_size(blk, size);

    blk->prev = prev;
    blk->next = next;
    blk->used = false;

    footer* foot = block_footer(blk);
    foot->start = blk;

#ifdef KHEAP_DEBUG
    blk->magic = BLOCK_HEADER_MAGIC;
    foot->magic = BLOCK_FOOTER_MAGIC;
#endif
}

void block_init_orphan(block* blk, u64 size) {
    block_init(blk, size, NULL, NULL);
}

u64 block_size(block* blk) { return blk->size << SIZE_UNUSED_BITS; }

void block_set_size(block* blk, u64 size) {
    blk->size = size >> SIZE_UNUSED_BITS;
}

void* block_free_space(block* blk) {
    return (void*) ((u64) blk + sizeof(header));
}

block* block_from_free_space(void* addr) {
    return (block*) ((u64) addr - sizeof(header));
}

void block_clear_node(block* blk) {
    blk->prev = NULL;
    blk->next = NULL;
}

block* block_left_adjacent(block* blk) {
    return ((footer*) ((u64) blk - sizeof(footer)))->start;
}

block* block_right_adjacent(block* blk) {
    return (block*) ((u64) blk + block_size(blk));
}

void block_print(block* blk) {
    print("(p: ");
    if (blk->prev) {
        print_u64_hex((u64) blk->prev);
    } else {
        print("NULL");
    }
    print(", n: ");
    if (blk->next) {
        print_u64_hex((u64) blk->next);
    } else {
        print("NULL");
    }
    print(", a: ");
    print_u64_hex((u64) blk);
    print(", ");
    print_u64(block_size(blk));
    print(blk->used ? ", u" : ", e");
    println(") -> ");
}