#include "kheap.h"

#include "../../arch/common/vmm.h"
#include "../../lib/alignment.h"
#include "../../lib/kprint.h"
#include "../../lib/math.h"
#include "bin.h"

// This is simplified version of Doug Lea`s malloc

#define BINS_COUNT 64

typedef struct {
    lock lock;
    bin bins[BINS_COUNT];
    u64 capacity;

    u64 allocs;
    u64 deallocs;
} heap;

typedef struct {
    block* aligned_block;
    u64 alignment_gap;
} best_fit;

static heap kheap;

void kfree_unsafe(void* addr);
bool grow_heap(u64 size);

bin* bin_for_size(u64 size);
bin* bin_for_block(block* blk);
best_fit find_best_fit_or_grow(u64 size, u64 alignment);

block* preceding_block(block* blk);
block* succeeding_block(block* blk);
block* shrink_block(bin* b, block* blk, u64 size);

void kheap_init() {
    memset(&kheap, 0, sizeof(kheap));
    grow_heap(KHEAP_INITIAL_SIZE);
    kheap.lock = SPIN_LOCK_STATIC_INITIALIZER;
}

void* kmalloc(u64 size) { return kmalloc_aligned(size, 8); }

// alignment must be multiple of 8
void* kmalloc_aligned(u64 size, u64 alignment) {
    u64 aligned_size = align_to_upper(
        MAX(size + sizeof(header) + sizeof(footer), MIN_BLOCK_SIZE), 8);

    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);

    best_fit best_fit = find_best_fit_or_grow(aligned_size, alignment);
    block* blk = best_fit.aligned_block;
    u64 gap = best_fit.alignment_gap;

    if (!blk) {
        spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
        return NULL;
    }

    block* result = gap == 0 ? blk : shrink_block(bin_for_block(blk), blk, gap);
    if (block_size(result) - aligned_size > MIN_BLOCK_SIZE)
        shrink_block(bin_for_block(result), result, aligned_size);

    bin_remove(bin_for_block(result), result);
    result->used = true;

    kheap.allocs++;
    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);

    return block_free_space(result);
}

void* krealloc(void* addr, u64 size) {
    block* old_block = block_from_free_space(addr);
    u64 to_copy =
        MIN(block_size(old_block) - sizeof(header) - sizeof(footer), size);

    void* new_data = kmalloc(size);
    if (!new_data)
        return NULL;

    memcpy(new_data, addr, to_copy);
    kfree(addr);

    return new_data;
}

void kfree(void* addr) {
    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);
    kfree_unsafe(addr);
    kheap.deallocs++;
    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
}

u64 kheap_size() {
    bool interrupts_enabled = spin_lock_irq_save(&kheap.lock);
    u64 size = kheap.capacity;
    spin_unlock_irq_restore(&kheap.lock, interrupts_enabled);
    return size;
}

void kfree_unsafe(void* addr) {
    block* blk = block_from_free_space(addr);
    if (block_size(blk) < MIN_BLOCK_SIZE || !blk->used)
        panic("Invalid block passed to free");

    block* preceding = preceding_block(blk);
    block* succeeding = succeeding_block(blk);

    vaddr start = (vaddr) blk;
    vaddr size = block_size(blk);

    if (preceding && !preceding->used) {
        start = (vaddr) preceding;
        size += block_size(preceding);
        bin_remove(bin_for_block(preceding), preceding);
    }

    if (succeeding && !succeeding->used) {
        size += block_size(succeeding);
        bin_remove(bin_for_block(succeeding), succeeding);
    }

    block* coalesced = (block*) start;
    block_init_orphan(coalesced, size);
    bin_insert(bin_for_block(coalesced), coalesced);
}

bool grow_heap(u64 size) {
    u64 aligned_size = align_to_upper(size, PAGE_SIZE);
    u64 start = KHEAP_START_VADDR + kheap.capacity;
    u64 end = start + aligned_size;

    u64 mapped = 0;
    for (u64 vframe = start; vframe < end; vframe += PAGE_SIZE) {
        vm_area_flags flags = {.writable = true};
        if (!arch_map_kernel_page(vframe, flags))
            break;

        mapped += PAGE_SIZE;
    }

    if (!mapped)
        return false;

    block* blk = (block*) start;
    block_init_orphan((block*) start, mapped);
    blk->used = true;
    kfree_unsafe(block_free_space(blk));

    kheap.capacity += mapped;
    return mapped == aligned_size;
}

bin* bin_for_size(u64 size) {
    return size == 0 ? NULL : &kheap.bins[msb_u64(size)];
}

bin* bin_for_block(block* blk) { return bin_for_size(block_size(blk)); }

block* preceding_block(block* blk) {
    return (vaddr) blk - sizeof(footer) > KHEAP_START_VADDR
               ? block_left_adjacent(blk)
               : NULL;
}

block* succeeding_block(block* blk) {
    return (vaddr) blk + block_size(blk) < KHEAP_START_VADDR + kheap.capacity
               ? block_right_adjacent(blk)
               : NULL;
}

best_fit block_fits_for_aligned_allocation(block* blk, u64 size,
                                           u64 alignment) {

    vaddr block_start = (vaddr) blk;
    vaddr free = (vaddr) block_free_space(blk);
    vaddr free_space_aligned = align_to_upper(free, alignment);
    vaddr block_end = block_start + block_size(blk);
    vaddr free_space_start = free_space_aligned;
    vaddr aligned_block_start = free_space_start - sizeof(header);

    u64 gap_size = aligned_block_start > block_start
                       ? aligned_block_start - block_start
                       : 0;
    u64 free_space_aligned_size = block_end - free_space_start;

    if (free == free_space_aligned && free_space_aligned_size >= size)
        return (best_fit){.aligned_block = blk, .alignment_gap = 0};

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

    if (free_space_aligned_size >= size && gap_size >= MIN_BLOCK_SIZE)
        return (best_fit){.aligned_block = blk, .alignment_gap = gap_size};

    return (best_fit){.aligned_block = NULL, .alignment_gap = 0};
}

best_fit find_best_fit_block_aligned(bin* bin, u64 size, u64 alignment) {
    block* cur = bin->first;
    best_fit res = {.aligned_block = NULL, .alignment_gap = 0};

    while (cur
           && !((res = block_fits_for_aligned_allocation(cur, size, alignment))
                    .aligned_block)) {
        cur = cur->next;
    }

    return res;
}

best_fit find_best_fit_aligned(u64 size, u64 alignment) {
    best_fit result = {.aligned_block = NULL, .alignment_gap = 0};
    block* blk = NULL;
    bin* b = bin_for_size(size);

    while (!blk && b <= &kheap.bins[BINS_COUNT - 1]) {
        result = find_best_fit_block_aligned(b, size, alignment);
        blk = result.aligned_block;
        b += !blk ? 1 : 0;
    }

    return (best_fit){.aligned_block = blk,
                      .alignment_gap = result.alignment_gap};
}

best_fit find_best_fit_or_grow(u64 size, u64 alignment) {
    while (true) {
        best_fit result = find_best_fit_aligned(size, alignment);

        if ((!result.aligned_block && !grow_heap(size))
            || result.aligned_block) {
            return result;
        }
    }
}

block* shrink_block(bin* b, block* blk, u64 size) {
    bin_remove(b, blk);

    block* right_split = (block*) ((vaddr) blk + size);
    block* left_split = blk;

    block_init_orphan(right_split, block_size(blk) - size);
    block_init_orphan(left_split, size);

    bin_insert(bin_for_block(left_split), left_split);
    bin_insert(bin_for_block(right_split), right_split);

    return right_split;
}