#include "kheap.h"

#include "../../lib/alignment.h"
#include "../../lib/memory_util.h"
#include "../../memory/memory_map.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"
#include "../../spin_lock.h"

typedef struct __attribute__((__packed__)) _header {
    volatile u64 magic;
    u64 size; // sizeof(header) + sizeof(footer) included
    bool used;
    struct _header* prev;
    struct _header* next;
} header;

typedef struct __attribute__((__packed__)) {
    volatile u64 magic;
    header* start;
} footer;

#define block header
#define BLOCK_ADDR(blk) ((vaddr) (blk))
#define FREE_SIZE(block_size) ((block_size) - sizeof(header) - sizeof(footer))
#define BLOCK_SIZE(free_size) ((free_size) + sizeof(header) + sizeof(footer))
#define BLOCK_FREE_SIZE(blk) (FREE_SIZE((blk)->size))

#define FREE_SPACE_FROM_BLOCK(blk) ((void*) ((vaddr) (blk) + sizeof(header)))
#define BLOCK_FROM_FREE_SPACE(free_space)                                      \
    ((block*) ((vaddr) (free_space) - sizeof(header)))
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
    u64 size;
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

block* preceding_block(block* blk) {
    vaddr prec_blk_footer_addr = (vaddr) (blk) - sizeof(footer);
    if (prec_blk_footer_addr < KHEAP_START_VADDR) {
        return NULL;
    }

    return ((footer*) prec_blk_footer_addr)->start;
}

block* succeeding_block(block* blk) {
    vaddr succ_blk_header_addr = (vaddr) blk + blk->size;
    if (succ_blk_header_addr >= KHEAP_START_VADDR + kheap.size) {
        return NULL;
    }

    return (block*) succ_blk_header_addr;
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
    head->used = false;
    head->prev = prev;
    head->next = next;
    head->magic = 0xDEADBEEF;

    footer* foot = (footer*) ((u64) head + head->size - sizeof(footer));
    foot->start = head;
    foot->magic = 0xCAFEBABE;

    return head;
}

void insert_block(bin* bin, block* to_insert) {
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

// bin -> prev -> cur -> next <- bin_last
block* remove_block(bin* block_bin, block* to_remove) {
    block* prev = to_remove->prev;
    block* next = to_remove->next;

    if (prev == NULL) {
        block_bin->first = next;
    } else {
        prev->next = next;
    }

    if (next == NULL) {
        block_bin->last = prev;
    } else {
        next->prev = prev;
    }
    block_bin->blocks--;

    to_remove->prev = NULL;
    to_remove->next = NULL;
    return to_remove;
}

typedef struct {
    block* left_split;
    block* right_split;
} split_result;

split_result split_block(bin* block_bin, block* to_split, u64 split_size) {
    block* prev = to_split->prev;
    block* next = to_split->next;

    u64 left_size = split_size;
    u64 right_size = to_split->size - split_size;

    block* left_split = to_split;
    block* right_split = (block*) ((vaddr) to_split + split_size);
    init_block((vaddr) left_split, left_size, prev, right_split);
    init_block((vaddr) right_split, right_size, left_split, next);

    if (prev == NULL) {
        block_bin->first = left_split;
    }
    if (next == NULL) {
        block_bin->last = right_split;
    }

    block_bin->blocks++;
    split_result result = {.left_split = left_split,
                           .right_split = right_split};
    return result;
}

block* cut_block(bin* block_bin, block* cut_from, u64 shrink_size) {
    u64 new_block_size = cut_from->size - shrink_size;
    if (new_block_size < MIN_BLOCK_SIZE) {
        return remove_block(block_bin, cut_from);
    }

    split_result splits = split_block(block_bin, cut_from, shrink_size);
    block* left_split = remove_block(block_bin, splits.left_split);
    block* right_split = splits.right_split;

    bin* right_split_bin = find_best_fit_bin(right_split->size);
    if (right_split_bin != block_bin) {
        right_split = remove_block(block_bin, right_split);
        insert_block(right_split_bin, right_split);
    }

    return left_split;
}

void* kmalloc(u64 size) {
    u64 block_size = align_to_upper(BLOCK_SIZE(size), 8);

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
    }

    block* result = cut_block(best_fit_bin, best_fit_block, block_size);
    result->used = true;

    return FREE_SPACE_FROM_BLOCK(result);
}

void kfree(void* addr) {
    block* to_free = BLOCK_FROM_FREE_SPACE(addr);
    block* coaelsed = to_free;

    block* preceding = preceding_block(coaelsed);
    if (preceding != NULL && preceding->used == false) {
        bin* preceding_bin = find_best_fit_bin(FREE_SIZE(preceding->size));
        remove_block(preceding_bin, preceding);
        init_block((vaddr) preceding, preceding->size + coaelsed->size, NULL,
                   NULL);
        coaelsed = preceding;
    }

    block* succeeding = succeeding_block(coaelsed);
    if (succeeding != NULL && succeeding->used == false) {
        bin* succeeding_bin = find_best_fit_bin(FREE_SIZE(succeeding->size));
        remove_block(succeeding_bin, succeeding);
        init_block((vaddr) coaelsed, succeeding->size + coaelsed->size, NULL,
                   NULL);
    }

    coaelsed->used = false;
    bin* coaelsed_block_bin = find_best_fit_bin(FREE_SIZE(coaelsed->size));
    // TODO: optimize so that we don't reinsert block if coaelsed version is
    //       already in right bin
    insert_block(coaelsed_block_bin, coaelsed);
}

void kmalloc_init() {
    init_lock(&kheap.lock);
    for (u8 i = 0; i < BINS_COUNT; i++) {
        kheap.bins[i].blocks = 0;
        kheap.bins[i].first = NULL;
        kheap.bins[i].blocks = NULL;
    }
    kheap.size = KHEAP_INITIAL_SIZE;

    allocate_initial_heap_array();
    block* initial_block =
        init_block(KHEAP_START_VADDR, KHEAP_INITIAL_SIZE, NULL, NULL);
    bin* initial_block_bin = find_best_fit_bin(FREE_SIZE(KHEAP_INITIAL_SIZE));
    insert_block(initial_block_bin, initial_block);
}
