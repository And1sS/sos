#include "kheap.h"

#include "../../lib/alignment.h"
#include "../../lib/math.h"
#include "../../lib/memory_util.h"
#include "../../memory/memory_map.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"
#include "../../spin_lock.h"

#define SIZE_UNUSED_BITS 3

typedef struct __attribute__((__packed__)) _header {
#ifdef KHEAP_DEBUG
    u64 magic;
#endif

    u64 size : 61; // since blocks are 8 bytes aligned, 3 lower bits are unused,
                   // so we can pack size.
                   // Should be touched only via BLOCK_SIZE macro.
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

#define BLOCK_ADDR(blk) ((vaddr) (blk))
#define BLOCK_SIZE(blk) ((blk)->size << SIZE_UNUSED_BITS)
#define FREE_SIZE_FROM_BLOCK_SIZE(block_size)                                  \
    ((block_size) - sizeof(header) - sizeof(footer))
#define BLOCK_SIZE_FROM_FREE_SIZE(free_size)                                   \
    ((free_size) + sizeof(header) + sizeof(footer))
#define BLOCK_FREE_SIZE(blk) (FREE_SIZE_FROM_BLOCK_SIZE(BLOCK_SIZE((blk))))

#define FREE_SPACE_FROM_BLOCK(blk) ((void*) ((vaddr) (blk) + sizeof(header)))
#define BLOCK_FROM_FREE_SPACE(free_space)                                      \
    ((block*) ((vaddr) (free_space) - sizeof(header)))
#define MIN_BLOCK_SIZE (2 * (sizeof(header) + sizeof(footer)))

typedef struct {
    block* first;
    u64 blocks;
} bin;

#define BINS_COUNT 64

typedef struct {
    lock lock;
    bin bins[BINS_COUNT];
    u64 capacity;
} heap;

typedef struct {
    block* left_split;
    block* right_split;
} split_result;

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

bin* find_best_fit_bin(u64 size) {
    return size == 0 ? NULL : &kheap.bins[most_significant_bit(size)];
}

block* preceding_block(block* blk) {
    vaddr prec_blk_footer_addr = (vaddr) (blk) - sizeof(footer);
    if (prec_blk_footer_addr < KHEAP_START_VADDR) {
        return NULL;
    }

    return ((footer*) prec_blk_footer_addr)->start;
}

block* succeeding_block(block* blk) {
    vaddr succ_blk_header_addr = (vaddr) blk + BLOCK_SIZE(blk);
    if (succ_blk_header_addr >= KHEAP_START_VADDR + kheap.capacity) {
        return NULL;
    }

    return (block*) succ_blk_header_addr;
}

block* find_best_fit_block(bin* bin, u64 size) {
    if (bin->blocks == 0) {
        return NULL;
    }

    block* cur = bin->first;
    while (cur != NULL) {
        if (BLOCK_SIZE(cur) > size) {
            return cur;
        }
    }

    return NULL;
}

block* init_block(vaddr addr, u64 size, header* prev, header* next) {
    header* head = (header*) addr;
    head->size = size >> SIZE_UNUSED_BITS;
    head->reserved = 0;
    head->used = false;
    head->prev = prev;
    head->next = next;

#ifdef KHEAP_DEBUG
    head->magic = 0xDEADBEEF;
#endif

    footer* foot = (footer*) ((u64) head + BLOCK_SIZE(head) - sizeof(footer));
    foot->start = head;

#ifdef KHEAP_DEBUG
    foot->magic = 0xCAFEBABE;
#endif

    return head;
}

void insert_block(bin* bin, block* to_insert) {
    if (bin->blocks == 0) {
        bin->blocks = 1;
        bin->first = to_insert;
        return;
    }

    block* first_block = bin->first;

    // (spot)a->b->c->null
    // case:
    if (BLOCK_SIZE(to_insert) < BLOCK_SIZE(first_block)) {
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

        while (cur != NULL && BLOCK_SIZE(to_insert) > BLOCK_SIZE(cur)) {
            prev = cur;
            cur = cur->next;
        }

        prev->next = to_insert;
        to_insert->next = cur;
        to_insert->prev = prev;

        if (cur != NULL) { // a->b->(spot)c->null
            cur->prev = to_insert;
        }
    }
    bin->blocks++;
}

block* remove_block(bin* block_bin, block* to_remove) {
    block* prev = to_remove->prev;
    block* next = to_remove->next;

    if (prev == NULL) {
        block_bin->first = next;
    } else {
        prev->next = next;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    block_bin->blocks--;

    to_remove->prev = NULL;
    to_remove->next = NULL;
    return to_remove;
}

split_result split_block(bin* block_bin, block* to_split, u64 split_size) {
    block* prev = to_split->prev;
    block* next = to_split->next;

    u64 left_size = split_size;
    u64 right_size = BLOCK_SIZE(to_split) - split_size;

    block* left_split = to_split;
    block* right_split = (block*) ((vaddr) to_split + split_size);
    init_block((vaddr) left_split, left_size, prev, right_split);
    init_block((vaddr) right_split, right_size, left_split, next);

    if (prev == NULL) {
        block_bin->first = left_split;
    }

    block_bin->blocks++;
    split_result result = {.left_split = left_split,
                           .right_split = right_split};
    return result;
}

void move_to_valid_bin(bin* block_bin, block* blk) {
    bin* valid_bin = find_best_fit_bin(BLOCK_SIZE(blk));
    if (valid_bin != block_bin) {
        blk = remove_block(block_bin, blk);
        insert_block(valid_bin, blk);
    }
}

block* cut_block(bin* block_bin, block* cut_from, u64 shrink_size) {
    u64 new_block_size = BLOCK_SIZE(cut_from) - shrink_size;
    if (new_block_size < MIN_BLOCK_SIZE) {
        return remove_block(block_bin, cut_from);
    }

    split_result splits = split_block(block_bin, cut_from, shrink_size);
    block* left_split = remove_block(block_bin, splits.left_split);
    block* right_split = splits.right_split;
    move_to_valid_bin(block_bin, right_split);

    return left_split;
}

block* coalesce(block* left, block* right) {
    // TODO: optimize so that we don't reinsert block if coaelsed version is
    //       already in right bin
    bin* left_bin = find_best_fit_bin(BLOCK_SIZE(left));
    remove_block(left_bin, left);
    bin* right_bin = find_best_fit_bin(BLOCK_SIZE(right));
    remove_block(right_bin, right);

    u64 coalesced_size = BLOCK_SIZE(left) + BLOCK_SIZE(right);
    init_block((vaddr) left, coalesced_size, NULL, NULL);
    return left;
}

void* kmalloc(u64 size) {
    u64 block_size = align_to_upper(BLOCK_SIZE_FROM_FREE_SIZE(size), 8);

    block* best_fit_block = NULL;
    bin* best_fit_bin = find_best_fit_bin(block_size);

    while (best_fit_block == NULL && IS_BIN_VALID(best_fit_bin)) {
        best_fit_block = find_best_fit_block(best_fit_bin, block_size);
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
    block* coalesced = BLOCK_FROM_FREE_SPACE(addr);
    block* preceding = preceding_block(coalesced);
    block* succeeding = succeeding_block(coalesced);

    if (preceding != NULL && preceding->used == false) {
        coalesced = coalesce(preceding, coalesced);
    }
    if (succeeding != NULL && succeeding->used == false) {
        coalesced = coalesce(coalesced, succeeding);
    }

    coalesced->used = false;
    bin* coalesced_block_bin = find_best_fit_bin(BLOCK_SIZE(coalesced));
    insert_block(coalesced_block_bin, coalesced);
}

void kmalloc_init() {
    init_lock(&kheap.lock);
    for (u8 i = 0; i < BINS_COUNT; i++) {
        kheap.bins[i].blocks = 0;
        kheap.bins[i].first = NULL;
        kheap.bins[i].blocks = NULL;
    }
    kheap.capacity = KHEAP_INITIAL_SIZE;

    allocate_initial_heap_array();
    block* initial_block =
        init_block(KHEAP_START_VADDR, KHEAP_INITIAL_SIZE, NULL, NULL);
    bin* initial_block_bin = find_best_fit_bin(KHEAP_INITIAL_SIZE);
    insert_block(initial_block_bin, initial_block);
}
