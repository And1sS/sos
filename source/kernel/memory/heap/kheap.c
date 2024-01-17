#include "kheap.h"

#include "../../arch/common/vmm.h"
#include "../../lib/alignment.h"
#include "../../lib/kprint.h"
#include "../../lib/math.h"
#include "../../lib/memory_util.h"
#include "../../synchronization/spin_lock.h"
#include "../memory_map.h"
#include "../physical/pmm.h"
#include "../virtual/vmm.h"

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

typedef struct {
    block* aligned_block;
    u64 alignment_gap;
} aligned_block_fit_result;

typedef struct {
    block* aligned_block;
    u64 alignment_gap;
    bin* aligned_block_bin;
} aligned_best_fit_result;

typedef struct {
    block* orphan;
    block* remaining;
    bin* remaining_bin;
} shrink_result;

static heap kheap;

void kfree_unsafe(void* addr);
bool grow_heap(u64 size);

bin* find_best_fit_bin(u64 size);
bin* find_best_fit_bin_for_block(block* blk);
block* find_best_fit_block(bin* bin, u64 size);
best_fit_result find_best_fit(u64 size);
best_fit_result find_best_fit_or_grow(u64 size);
aligned_best_fit_result find_best_fit_or_grow_aligned(u64 size, u64 alignment);

void insert_block(bin* bin, block* to_insert);
block* remove_block(bin* block_bin, block* to_remove);

block* init_block(vaddr addr, u64 size, header* prev, header* next);
block* init_orphan_block(vaddr addr, u64 size);
u64 block_size(block* blk);
block* preceding_block(block* blk);
block* succeeding_block(block* blk);
split_result split_block(bin* block_bin, block* to_split, u64 split_size);
bin* move_to_valid_bin(bin* block_bin, block* blk);
shrink_result shrink_block(bin* block_bin, block* cut_from, u64 shrink_size);
block* coalesce_blocks(block* left, block* right);

void* free_space(block* blk);

void kheap_init() {
    memset(&kheap, 0, sizeof(kheap));
    grow_heap(KHEAP_INITIAL_SIZE);
    kheap.lock = SPIN_LOCK_STATIC_INITIALIZER;
    kheap.capacity = align_to_upper(KHEAP_INITIAL_SIZE, PAGE_SIZE);
}

void* kmalloc(u64 size) {
    u64 block_size = align_to_upper(
        MAX(size + sizeof(header) + sizeof(footer), MIN_BLOCK_SIZE), 8);

    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);

    best_fit_result best_fit = find_best_fit_or_grow(block_size);
    block* blk = best_fit.blk;
    if (!blk) {
        spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);

        return NULL;
    }

    shrink_result result = shrink_block(best_fit.blk_bin, blk, block_size);
    block* result_block = result.orphan;
    result_block->used = true;

    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);

    return free_space(result_block);
}

// alignment must be multiple of 8
void* kmalloc_aligned(u64 size, u64 alignment) {
    u64 block_size = align_to_upper(
        MAX(size + sizeof(header) + sizeof(footer), MIN_BLOCK_SIZE), 8);

    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);

    aligned_best_fit_result best_fit =
        find_best_fit_or_grow_aligned(block_size, alignment);

    block* blk = best_fit.aligned_block;
    if (!blk) {
        spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);

        return NULL;
    }

    u64 alignment_gap = best_fit.alignment_gap;
    bin* blk_bin = best_fit.aligned_block_bin;

    if (alignment_gap == 0) {
        block* result_block = shrink_block(blk_bin, blk, block_size).orphan;
        result_block->used = true;
        spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);

        return free_space(result_block);
    }

    shrink_result gap_result = shrink_block(blk_bin, blk, alignment_gap);
    block* gap_block = gap_result.orphan;
    block* remainder = gap_result.remaining;
    bin* remainder_bin = gap_result.remaining_bin;

    shrink_result result = shrink_block(remainder_bin, remainder, block_size);
    insert_block(find_best_fit_bin_for_block(gap_block), gap_block);

    block* result_block = result.orphan;
    result_block->used = true;

    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);

    return free_space(result_block);
}

void* krealloc(void* addr, u64 size) {
    block* old_block = (block*) ((vaddr) addr - sizeof(header));
    u64 old_data_size = old_block->size - sizeof(header) - sizeof(footer);

    u64 to_copy = MIN(old_data_size, size);
    void* new_data = kmalloc(size);
    memcpy(new_data, addr, to_copy);
    kfree(addr);

    return new_data;
}

void kfree(void* addr) {
    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);
    kfree_unsafe(addr);
    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
}

u64 kheap_size() {
    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);
    u64 size = kheap.capacity;
    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
    return size;
}

void kfree_unsafe(void* addr) {
    block* coalesced = (block*) ((vaddr) addr - sizeof(header));
    if (block_size(coalesced) < MIN_BLOCK_SIZE || !coalesced->used)
        panic("Invalid block passed to free");

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
}

bool grow_heap(u64 size) {
    u64 aligned_size = align_to_upper(size, PAGE_SIZE);
    u64 start = KHEAP_START_VADDR + kheap.capacity;
    u64 end = start + aligned_size;

    for (u64 vframe = start; vframe < end; vframe += PAGE_SIZE) {
        vm_area_flags flags = {.present = true, .writable = true};
        arch_map_kernel_page(vframe, flags);
    }

    memset((void*) start, 0, aligned_size);

    block* blk = init_orphan_block(start, aligned_size);
    blk->used = true;
    kfree_unsafe(free_space(blk));

    kheap.capacity += aligned_size;
    return true;
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

aligned_block_fit_result block_fits_for_aligned_allocation(block* blk, u64 size,
                                                           u64 alignment) {

    vaddr block_start = (vaddr) blk;
    vaddr free = (vaddr) free_space(blk);
    vaddr free_space_aligned = align_to_upper(free, alignment);
    vaddr block_end = block_start + block_size(blk);
    vaddr free_space_start = free_space_aligned;
    vaddr aligned_block_start = free_space_start - sizeof(header);

    u64 gap_size = aligned_block_start > block_start
                       ? aligned_block_start - block_start
                       : 0;
    u64 free_space_aligned_size = block_end - free_space_start;

    if (free == free_space_aligned && free_space_aligned_size >= size) {
        aligned_block_fit_result result = {.aligned_block = blk,
                                           .alignment_gap = 0};
        return result;
    }

    while (free_space_start < block_end && free_space_aligned_size > size
           && gap_size < MIN_BLOCK_SIZE) {
        free_space_start += alignment;

        aligned_block_start = free_space_start - sizeof(header);
        gap_size = aligned_block_start > block_start
                       ? aligned_block_start - block_start
                       : 0;
        free_space_aligned_size =
            block_end > free_space_start ? block_end - free_space_start : 0;
    }

    if (free_space_aligned_size >= size && gap_size >= MIN_BLOCK_SIZE) {
        aligned_block_fit_result result = {.aligned_block = blk,
                                           .alignment_gap = gap_size};
        return result;
    }

    aligned_block_fit_result result = {.aligned_block = NULL,
                                       .alignment_gap = 0};
    return result;
}

aligned_block_fit_result find_best_fit_block_aligned(bin* bin, u64 size,
                                                     u64 alignment) {
    block* cur = bin->first;
    aligned_block_fit_result res = {.aligned_block = NULL, .alignment_gap = 0};

    while (cur
           && !((res = block_fits_for_aligned_allocation(cur, size, alignment))
                    .aligned_block)) {
        cur = cur->next;
    }

    return res;
}

block* find_best_fit_block(bin* bin, u64 size) {
    block* cur = bin->first;
    while (cur && block_size(cur) <= size) {
        cur = cur->next;
    }

    return cur;
}

aligned_best_fit_result find_best_fit_aligned(u64 size, u64 alignment) {
    aligned_block_fit_result best_fit = {.aligned_block = NULL,
                                         .alignment_gap = 0};
    block* best_fit_block = NULL;
    bin* best_fit_bin = find_best_fit_bin(size);

    while (!best_fit_block && best_fit_bin <= &kheap.bins[BINS_COUNT - 1]) {
        best_fit = find_best_fit_block_aligned(best_fit_bin, size, alignment);
        best_fit_block = best_fit.aligned_block;
        best_fit_bin += !best_fit_block ? 1 : 0;
    }

    aligned_best_fit_result result = {.aligned_block = best_fit_block,
                                      .alignment_gap = best_fit.alignment_gap,
                                      .aligned_block_bin = best_fit_bin};

    return result;
}

best_fit_result find_best_fit(u64 size) {
    block* best_fit_block = NULL;
    bin* best_fit_bin = find_best_fit_bin(size);

    while (!best_fit_block && best_fit_bin <= &kheap.bins[BINS_COUNT - 1]) {
        best_fit_block = find_best_fit_block(best_fit_bin, size);
        best_fit_bin += !best_fit_block ? 1 : 0;
    }

    best_fit_result result = {.blk = best_fit_block,
                              .blk_bin = best_fit_block ? best_fit_bin : NULL};

    return result;
}

aligned_best_fit_result find_best_fit_or_grow_aligned(u64 size, u64 alignment) {
    aligned_best_fit_result result;

    while (true) {
        result = find_best_fit_aligned(size, alignment);

        if ((!result.aligned_block && !grow_heap(size))
            || result.aligned_block) {
            return result;
        }
    }
}

best_fit_result find_best_fit_or_grow(u64 size) {
    while (true) {
        best_fit_result best_fit = find_best_fit(size);

        if ((!best_fit.blk && !grow_heap(size)) || best_fit.blk) {
            return best_fit;
        }
    }
}

void insert_block(bin* bin, block* to_insert) {
    if (!bin->first) {
        bin->first = to_insert;
        return;
    }

    block* first_block = bin->first;

    // (spot)a->b->c->null
    // case:
    if (block_size(to_insert) <= block_size(first_block)) {
        to_insert->next = first_block;
        first_block->prev = to_insert;
        bin->first = to_insert;
    }
    // a->b->(spot)c->null
    // a->b->c->(spot)null
    // cases:
    else {
        block* prev = NULL;
        block* cur = first_block;

        while (cur && block_size(to_insert) > block_size(cur)) {
            prev = cur;
            cur = cur->next;
        }

        if (prev) {
            prev->next = to_insert;
        } else {
            bin->first = to_insert;
        }
        to_insert->next = cur;
        to_insert->prev = prev;

        if (cur) { // a->b->(spot)c->null
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

    if (!prev) {
        block_bin->first = left_split;
    }

#ifdef KHEAP_DEBUG
    block_bin->blocks++;
#endif

    split_result result = {.left_split = left_split,
                           .right_split = right_split};
    return result;
}

bin* move_to_valid_bin(bin* block_bin, block* blk) {
    bin* valid_bin = find_best_fit_bin_for_block(blk);
    if (valid_bin != block_bin) {
        blk = remove_block(block_bin, blk);
        insert_block(valid_bin, blk);
    }

    return valid_bin;
}

shrink_result shrink_block(bin* block_bin, block* cut_from, u64 shrink_size) {
    shrink_result result = {
        .orphan = NULL, .remaining = NULL, .remaining_bin = NULL};

    u64 new_block_size = block_size(cut_from) - shrink_size;
    if (new_block_size < MIN_BLOCK_SIZE) {
        result.orphan = remove_block(block_bin, cut_from);
        return result;
    }

    split_result splits = split_block(block_bin, cut_from, shrink_size);
    block* left_split = remove_block(block_bin, splits.left_split);
    block* right_split = splits.right_split;
    bin* right_split_bin = move_to_valid_bin(block_bin, right_split);

    result.orphan = left_split;
    result.remaining = right_split;
    result.remaining_bin = right_split_bin;

    return result;
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

void* free_space(block* blk) { return (void*) ((vaddr) blk + sizeof(header)); }

void kheap_print() {
    println("----------------------");
    println("BINS:");
    for (int i = 7; i < 24; ++i) {
        bin* b = &kheap.bins[i];
        print("bin #");
        print_u32(i);
        print(" -> ");
        block* blk = b->first;
        while (blk) {
            print("(");
            print_u64_hex((u64) blk);
            print(", ");
            print_u32(block_size(blk));
            print(") -> ");
            blk = blk->next;
        }
        println("NULL");
    }

    println("BLOCKS:");
    block* blk = (block*) KHEAP_START_VADDR;
    while (blk && (u64) blk < KHEAP_START_VADDR + kheap.capacity) {
        print("(");
        print_u64_hex((u64) blk);
        print(", ");
        print_u32(block_size(blk));
        print(", ");
        print(blk->used ? "used" : "unused");
        print(");");
        blk = (block*) ((u64) blk + block_size(blk));
    }
    println("");
    println("----------------------");
}