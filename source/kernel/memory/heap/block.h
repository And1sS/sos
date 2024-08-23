#ifndef SOS_KHEAP_BLOCK_H
#define SOS_KHEAP_BLOCK_H

#include "../../lib/types.h"

#define BLOCK_HEADER_MAGIC 0xDEADBEEF
#define BLOCK_FOOTER_MAGIC 0xCAFEBABE

#define SIZE_UNUSED_BITS 3

typedef struct __attribute__((__packed__)) _header {
#ifdef KHEAP_DEBUG
    u64 magic;
#endif

    u64 size : 61; // Since blocks are 8 bytes aligned, 3 lower bits are unused,
                   // so we can pack size.
                   // Should be touched only via block_size function.
                   // sizeof(header) + sizeof(footer) included

    u8 reserved : 2;
    bool used : 1;

    struct _header* prev;
    struct _header* next;
} header;

typedef struct __attribute__((__packed__)) {
#ifdef KHEAP_DEBUG
    u64 magic;
#endif

    header* start;
} footer;

#define block header

#define MIN_BLOCK_SIZE (2 * (sizeof(header) + sizeof(footer)))

footer* block_footer(block* blk);

void block_init(block* blk, u64 size, block* prev, block* next);
void block_init_orphan(block* blk, u64 size);

u64 block_size(block* blk);
void block_set_size(block* blk, u64 size);
void* block_free_space(block* blk);
block* block_from_free_space(void* addr);

void block_clear_node(block* blk);

block* block_left_adjacent(block* blk);
block* block_right_adjacent(block* blk);

void block_print(block* blk);

#endif // SOS_KHEAP_BLOCK_H
