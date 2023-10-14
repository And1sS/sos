#include "kheap.h"

#include "../../lib/memory_util.h"
#include "../../memory/memory_map.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"
#include "../../spin_lock.h"

typedef struct _header {
    u64 size; // sizeof(header) + sizeof(footer) included
    struct _header* prev;
    struct _header* next;
} header;

typedef struct {
    header* start;
} footer;

#define block header
#define BLOCK_ADDR(blk) ((u64) blk)
#define FREE_SIZE(block_size) ((block_size) - sizeof(header) - sizeof(footer))
#define BLOCK_SIZE(free_size) ((free_size) + sizeof(header) + sizeof(footer))
#define BLOCK_FREE_SIZE(blk) (FREE_SIZE((blk)->size))
#define MIN_BLOCK_SIZE (2 * (sizeof(header) + sizeof(footer)))

typedef struct {
    block* first;
    block* last;
    u64 blocks;
} bin;

#define BINS_COUNT 64

typedef struct {
    lock lock;
    bin bins[BINS_COUNT];
} heap;

static heap kheap;

#define BIN_ENTRY_MAX_SIZE(bin_num) ((u64) 1 << (bin_num))
#define NEXT_BIN(bin_ptr) ((bin_ptr + 1))
#define IS_BIN_VALID(bin_ptr)                                                  \
    ((u64) (bin_ptr) <= (u64) &kheap.bins[BINS_COUNT - 1])                     \
        && ((u64) (bin_ptr) >= (u64) &kheap.bins[0])

void allocate_initial_heap_array() {
    for (u64 vframe = KHEAP_START_VADDR;
         vframe < KHEAP_START_VADDR + KHEAP_INITIAL_SIZE; vframe++) {

        u64 page = get_page(vframe);
        if (!(page & 1)) {
            paddr pframe = allocate_frame();
            map_page(vframe, pframe, 1 | 2 | 4);
        }
    }
    memset((void*) KHEAP_START_VADDR, 0, KHEAP_INITIAL_SIZE);
}

bin* find_best_fit_bin(u64 free_size) {
    // TODO: probably replace with binary search
    for (u8 bin = 0; bin < BINS_COUNT; bin++) {
        if (BLOCK_SIZE(free_size) < BIN_ENTRY_MAX_SIZE(bin)) {
            return &kheap.bins[bin];
        }
    }

    return NULL;
}

block* find_best_fit_block(bin* bin, u64 free_size) {
    if (bin->blocks == 0) {
        return NULL;
    }

    block* cur = bin->first;
    while (cur != NULL) {
        if (BLOCK_FREE_SIZE(cur) > free_size) {
            return cur;
        }
    }

    return NULL;
}

block* init_block(vaddr addr, u64 size, header* prev, header* next) {
    header* head = (header*) addr;
    head->size = size;
    head->prev = prev;
    head->next = next;

    footer* foot = (footer*) (addr + size - sizeof(footer));
    foot->start = head;

    return head;
}

void insert_into_bin(bin* bin, block* to_insert) {
    if (bin->blocks == 0) {
        bin->blocks = 1;
        bin->first = to_insert;
        bin->last = to_insert;
        return;
    }

    block* first_block = bin->first;

    // (spot)a->b->c->null
    // case:
    if (to_insert->size < first_block->size) {
        to_insert->next = first_block;
        first_block->prev = to_insert;
        bin->first = to_insert;
    }
    // a->b->(spot)c->null
    // a->b->c->(spot)null
    // cases:
    else {
        block* prev = first_block;
        block* cur = first_block;

        while (cur != NULL && to_insert->size > cur->size) {
            prev = cur;
            cur = cur->next;
        }

        prev->next = to_insert;
        to_insert->next = cur;
        to_insert->prev = prev;

        if (cur != NULL) { // a->b->(spot)c->null
            cur->prev = to_insert;
        } else { // a->b->c->(spot)null
            bin->last = to_insert;
        }
    }
    bin->blocks++;
}

#include "../../util.h"

block* shrink_block(bin* block_bin, block* to_shrink, u64 shrink_size) {
    UNUSED(block_bin);
    u64 new_block_size = to_shrink->size - shrink_size;
    if (new_block_size < MIN_BLOCK_SIZE)
        return to_shrink;

    return NULL;
}

void* kmalloc(u64 size) {
    block* best_fit_block = NULL;
    bin* best_fit_bin = find_best_fit_bin(size);

    while (best_fit_block == NULL && IS_BIN_VALID(best_fit_bin)) {
        best_fit_block = find_best_fit_block(best_fit_bin, size);
        if (best_fit_block == NULL) {
            best_fit_bin = NEXT_BIN(best_fit_bin);
        }
    }

    if (best_fit_block == NULL) {
        return NULL; // TODO: try growing heap
    } else {
    }

    return (void*) KHEAP_START_VADDR;
}

void kfree(void* addr) { UNUSED(addr); }

void kmalloc_init() {
    init_lock(&kheap.lock);
    for (u8 i = 0; i < BINS_COUNT; i++) {
        kheap.bins[i].blocks = 0;
        kheap.bins[i].first = NULL;
        kheap.bins[i].blocks = NULL;
    }

    allocate_initial_heap_array();
    block* initial_block =
        init_block(KHEAP_START_VADDR, KHEAP_INITIAL_SIZE, NULL, NULL);
    bin* initial_block_bin = find_best_fit_bin(FREE_SIZE(KHEAP_INITIAL_SIZE));
    insert_into_bin(initial_block_bin, initial_block);
}
