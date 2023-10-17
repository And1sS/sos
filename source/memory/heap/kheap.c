#include "kheap.h"

#include "../../lib/alignment.h"
#include "../../lib/math.h"
#include "../../lib/memory_util.h"
#include "../../memory/memory_map.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"
#include "../../spin_lock.h"

// This is simplified version of Doug Lea`s malloc

#define SIZE_UNUSED_BITS 3

#ifdef KHEAP_DEBUG
#define BLOCK_HEADER_MAGIC 0xDEADBEEF
#define BLOCK_FOOTER_MAGIC 0xCAFEBABE
#endif

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

typedef struct {
    block* first;

#ifdef KHEAP_DEBUG
    u64 blocks;
#endif
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

typedef struct {
    block* blk;
    bin* blk_bin;
} best_fit_result;

static heap kheap;

void grow_heap(u64 size);

bin* find_best_fit_bin(u64 size);
bin* find_best_fit_bin_for_block(block* blk);
block* find_best_fit_block(bin* bin, u64 size);
best_fit_result find_best_fit_or_grow(u64 size);

void insert_block(bin* bin, block* to_insert);
block* remove_block(bin* block_bin, block* to_remove);

block* init_block(vaddr addr, u64 size, header* prev, header* next);
block* init_orphan_block(vaddr addr, u64 size);
u64 block_size(block* blk);
block* preceding_block(block* blk);
block* succeeding_block(block* blk);
split_result split_block(bin* block_bin, block* to_split, u64 split_size);
void move_to_valid_bin(bin* block_bin, block* blk);
block* shrink_block(bin* block_bin, block* cut_from, u64 shrink_size);
block* coalesce_blocks(block* left, block* right);

void kmalloc_init() {
    memset(&kheap, 0, sizeof(kheap));
    init_lock(&kheap.lock);

    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);
    grow_heap(KHEAP_INITIAL_SIZE);
    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
}

void* kmalloc(u64 size) {
    u64 block_size = align_to_upper(size + sizeof(header) + sizeof(footer), 8);
    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);

    best_fit_result best_fit = find_best_fit_or_grow(block_size);
    block* result = shrink_block(best_fit.blk_bin, best_fit.blk, block_size);
    result->used = true;

    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
    return (void*) ((vaddr) result + sizeof(header));
}

void kfree(void* addr) {
    block* coalesced = (block*) ((vaddr) addr - sizeof(header));
    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);

    block* preceding = preceding_block(coalesced);
    block* succeeding = succeeding_block(coalesced);

    if (preceding != NULL && !preceding->used) {
        coalesced = coalesce_blocks(preceding, coalesced);
    }
    if (succeeding != NULL && !succeeding->used) {
        coalesced = coalesce_blocks(coalesced, succeeding);
    }

    coalesced->used = false;
    bin* coalesced_block_bin = find_best_fit_bin_for_block(coalesced);
    insert_block(coalesced_block_bin, coalesced);

    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
}

void grow_heap(u64 size) {
    u64 start = KHEAP_START_VADDR + kheap.capacity;
    u64 end = start + size;

    for (u64 vframe = start; vframe < end; vframe++) {
        u64 page = get_page(vframe);
        if (!(page & 1)) {
            paddr pframe = allocate_frame();
            map_page(vframe, pframe, 1 | 2 | 4);
        }
    }
    memset((void*) start, 0, size);

    init_orphan_block(start, size);
    block* blk = (block*) start;
    bin* block_bin = find_best_fit_bin(size);
    insert_block(block_bin, blk);

    kheap.capacity += size;
}

bin* find_best_fit_bin(u64 size) {
    return size == 0 ? NULL : &kheap.bins[msb_u64(size)];
}

bin* find_best_fit_bin_for_block(block* blk) {
    return find_best_fit_bin(block_size(blk));
}

block* init_block(vaddr addr, u64 size, header* prev, header* next) {
    header* head = (header*) addr;
    head->size = size >> SIZE_UNUSED_BITS;
    head->reserved = 0;
    head->used = false;
    head->prev = prev;
    head->next = next;

#ifdef KHEAP_DEBUG
    head->magic = BLOCK_HEADER_MAGIC;
#endif

    footer* foot = (footer*) ((u64) head + block_size(head) - sizeof(footer));
    foot->start = head;

#ifdef KHEAP_DEBUG
    foot->magic = BLOCK_FOOTER_MAGIC;
#endif

    return head;
}

block* init_orphan_block(vaddr addr, u64 size) {
    return init_block(addr, size, NULL, NULL);
}

u64 block_size(block* blk) { return blk->size << SIZE_UNUSED_BITS; }

block* preceding_block(block* blk) {
    vaddr prec_blk_footer_addr = (vaddr) (blk) - sizeof(footer);
    if (prec_blk_footer_addr < KHEAP_START_VADDR) {
        return NULL;
    }

    return ((footer*) prec_blk_footer_addr)->start;
}

block* succeeding_block(block* blk) {
    vaddr succ_blk_header_addr = (vaddr) blk + block_size(blk);
    if (succ_blk_header_addr >= KHEAP_START_VADDR + kheap.capacity) {
        return NULL;
    }

    return (block*) succ_blk_header_addr;
}

block* find_best_fit_block(bin* bin, u64 size) {
    block* cur = bin->first;
    while (cur != NULL && block_size(cur) <= size) {
        cur = cur->next;
    }

    return cur;
}

best_fit_result find_best_fit_or_grow(u64 size) {
    block* best_fit_block = NULL;
    bin* best_fit_bin = find_best_fit_bin(size);

    while (best_fit_block == NULL
           && best_fit_bin <= &kheap.bins[BINS_COUNT - 1]) {

        best_fit_block = find_best_fit_block(best_fit_bin, size);
        if (best_fit_block == NULL) {
            best_fit_bin++;
        }
    }

    if (best_fit_block == NULL) {
        // TODO: add kheap limit checks
        grow_heap(kheap.capacity / 2);
        return find_best_fit_or_grow(size);
    }

    best_fit_result result = {.blk = best_fit_block, .blk_bin = best_fit_bin};
    return result;
}

void insert_block(bin* bin, block* to_insert) {
    if (bin->first == NULL) {
        bin->first = to_insert;
        return;
    }

    block* first_block = bin->first;

    // (spot)a->b->c->null
    // case:
    if (block_size(to_insert) < block_size(first_block)) {
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

        while (cur != NULL && block_size(to_insert) > block_size(cur)) {
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
#ifdef KHEAP_DEBUG
    bin->blocks++;
#endif
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
#ifdef KHEAP_DEBUG
    block_bin->blocks--;
#endif
    to_remove->prev = NULL;
    to_remove->next = NULL;
    return to_remove;
}

split_result split_block(bin* block_bin, block* to_split, u64 split_size) {
    block* prev = to_split->prev;
    block* next = to_split->next;

    u64 left_size = split_size;
    u64 right_size = block_size(to_split) - split_size;

    block* left_split = to_split;
    block* right_split = (block*) ((vaddr) to_split + split_size);
    init_block((vaddr) left_split, left_size, prev, right_split);
    init_block((vaddr) right_split, right_size, left_split, next);

    if (prev == NULL) {
        block_bin->first = left_split;
    }

#ifdef KHEAP_DEBUG
    block_bin->blocks++;
#endif

    split_result result = {.left_split = left_split,
                           .right_split = right_split};
    return result;
}

void move_to_valid_bin(bin* block_bin, block* blk) {
    bin* valid_bin = find_best_fit_bin_for_block(blk);
    if (valid_bin != block_bin) {
        blk = remove_block(block_bin, blk);
        insert_block(valid_bin, blk);
    }
}

block* shrink_block(bin* block_bin, block* cut_from, u64 shrink_size) {
    u64 new_block_size = block_size(cut_from) - shrink_size;
    if (new_block_size < MIN_BLOCK_SIZE) {
        return remove_block(block_bin, cut_from);
    }

    split_result splits = split_block(block_bin, cut_from, shrink_size);
    block* left_split = remove_block(block_bin, splits.left_split);
    block* right_split = splits.right_split;
    move_to_valid_bin(block_bin, right_split);

    return left_split;
}

block* coalesce_blocks(block* left, block* right) {
    // TODO: optimize so that we don't reinsert block if coalesced version is
    //       already in right bin
    bin* left_bin = find_best_fit_bin_for_block(left);
    if (!left->used) {
        remove_block(left_bin, left);
    }
    bin* right_bin = find_best_fit_bin_for_block(right);
    if (!right->used) {
        remove_block(right_bin, right);
    }

    u64 coalesced_size = block_size(left) + block_size(right);
    init_orphan_block((vaddr) left, coalesced_size);
    return left;
}
