#include "vm.h"
#include "../../arch/common/vmm.h"
#include "../../lib/kprint.h"
#include "../../lib/math.h"
#include "../heap/kheap.h"

#define PAGE(base) (base & ~((u64) PAGE_SIZE - 1))

static vm_area* vm_area_clone(vm_area* area) {
    vm_area* clone = (vm_area*) kmalloc(sizeof(vm_area));
    if (!clone)
        return NULL;

    *clone = *area;

    return clone;
}

static void vm_area_validate(vm_area* area) {
    if (area->base % PAGE_SIZE != 0)
        panic("Area of non-page-aligned base");
    if (!area->length || area->length % PAGE_SIZE != 0)
        panic("Area of non-page-aligned length");
}

static bool vm_area_flags_equal(vm_area* left, vm_area* right) {
    vm_area_flags lflags = left->flags;
    vm_area_flags rflags = right->flags;

    return lflags.present == rflags.present
           && lflags.writable == rflags.writable
           && lflags.executable == rflags.executable
           && lflags.shared == rflags.shared
           && lflags.user_access_allowed == rflags.user_access_allowed;
}

static bool vm_area_before(vm_area* left, vm_area* right) {
    return left->base + left->length <= right->base;
}

static bool vm_area_contains_address(vm_area* area, u64 address) {
    return area->base <= address && address < area->base + area->length;
}

static bool vm_areas_intersect(vm_area* left, vm_area* right) {
    return vm_area_contains_address(left, right->base)
           || vm_area_contains_address(right, left->base);
}

static bool vm_area_contains_area(vm_area* left, vm_area* right) {
    return left->base <= right->base
           && right->base + right->length <= left->base + left->length;
}

static bool vm_areas_next_to_each_other(vm_area* left, vm_area* right) {
    return left->base + left->length == right->base
           || right->base + right->length == left->base;
}

static void vm_areas_merge(vm_area* left, vm_area* right) {
    left->base = MIN(left->base, right->base);
    u64 end = MAX(left->base + left->length, right->base + right->length);
    left->length = end - left->base;
}

static bool vm_areas_can_merge(vm_area* left, vm_area* right) {
    bool intersect = vm_areas_intersect(left, right);
    bool flags_equal = vm_area_flags_equal(left, right);
    if (intersect && !flags_equal)
        panic("Attempted to merge memory areas with different flags");

    bool can_merge = intersect || vm_areas_next_to_each_other(left, right);
    return can_merge && flags_equal;
}

static vm_area* vm_space_intersecting_area_unsafe(vm_space* space,
                                                  vm_area* area) {

    ARRAY_LIST_FOR_EACH(&space->areas, vm_area * iter) {
        if (vm_areas_intersect(iter, area)) {
            return area;
        }
    }

    return NULL;
}

static vm_area* vm_space_surrounding_area_unsafe(vm_space* space,
                                                 vm_area* area) {

    ARRAY_LIST_FOR_EACH(&space->areas, vm_area * iter) {
        if (vm_area_contains_area(iter, area)) {
            return area;
        }
    }

    return NULL;
}

bool vm_space_insert_area_unsafe(vm_space* space, vm_area* to_insert) {
    vm_area_validate(to_insert);

    bool merged = false;
    vm_area* prev = NULL;
    u64 next_idx = 0;

    vm_area* next = array_list_get(&space->areas, next_idx);

    while (next && vm_area_before(next, to_insert)) {
        prev = next;
        next = array_list_get(&space->areas, ++next_idx);
    }

    bool merge_prev = prev && vm_areas_can_merge(prev, to_insert);
    if (merge_prev) {
        vm_areas_merge(prev, to_insert); // prev holds merged area
        kfree(to_insert);
        merged = true;
    }

    bool merge_both = next && merge_prev && vm_areas_can_merge(prev, next);
    if (merge_both) {
        vm_areas_merge(prev, next); // prev holds merged area
        kfree(next);
        array_list_remove_idx(&space->areas, next_idx);
        merged = true;
    }

    bool merge_next =
        next && !merge_prev && vm_areas_can_merge(next, to_insert);
    if (merge_next) {
        vm_areas_merge(next, to_insert); // next holds merged area
        kfree(to_insert);
        merged = true;
    }

    if (!merge_prev && !merge_both) {
        merged = array_list_insert(&space->areas, next_idx, to_insert);
    }

    return merged;
}

bool vm_space_cut_area_unsafe(vm_space* space, vm_area* to_cut) {
    vm_area_validate(to_cut);

    u64 idx = 0;
    vm_area* curr = array_list_get(&space->areas, idx);
    while (curr && !vm_area_contains_area(curr, to_cut)) {
        curr = array_list_get(&space->areas, ++idx);
    }

    if (!curr || curr->base + curr->length < to_cut->base + to_cut->length)
        panic("Trying to cut non-existing area");

    if (curr->base == to_cut->base && curr->length == to_cut->length) {
        array_list_remove_idx(&space->areas, idx);
        kfree(curr);
        return true;
    } else if (curr->base == to_cut->base) {
        u64 old_end = curr->base + curr->length;
        u64 new_length = curr->length - to_cut->length;
        curr->length = new_length;
        curr->base = old_end - new_length;
        return true;
    } else if (curr->base + curr->length == to_cut->base + to_cut->length) {
        u64 new_length = curr->length - to_cut->length;
        curr->length = new_length;
        return true;
    } else {
        vm_area* right_remainder = (vm_area*) kmalloc(sizeof(vm_area*));
        if (!right_remainder)
            return false;

        u64 cut_end = to_cut->base + to_cut->length;
        right_remainder->base = cut_end;
        right_remainder->length = curr->base + curr->length - cut_end;
        right_remainder->flags = curr->flags;

        if (!array_list_insert(&space->areas, idx + 1, right_remainder)) {
            kfree(right_remainder);
            return false;
        }

        curr->length = to_cut->base - curr->base;
        return true;
    }
}

vm_space* vm_space_fork(vm_space* space) {
    rw_spin_lock_read_irq(&space->lock);

    vm_space* forked = (vm_space*) kmalloc(sizeof(vm_space));
    if (!forked) {
        rw_spin_unlock_read_irq(&space->lock);
        return NULL;
    }

    forked->is_kernel_space = false;
    forked->lock = (rw_spin_lock) RW_LOCK_STATIC_INITIALIZER;
    forked->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    ref_acquire(&forked->refc);

    // don't clone areas if we are forking kernel space, since we don't own them
    // and won't clone them
    if (space->is_kernel_space) {
        if (!array_list_init(&forked->areas, 0))
            goto areas_list_initialization_failed;
    } else {
        if (!array_list_init(&forked->areas, space->areas.capacity))
            goto areas_list_initialization_failed;

        ARRAY_LIST_FOR_EACH(&space->areas, vm_area * area) {
            vm_area* cloned = vm_area_clone(area);
            if (!cloned)
                goto area_clone_failed;

            if (!array_list_add_last(&forked->areas, cloned))
                goto area_clone_failed;
        }
    }

    forked->table = arch_fork_page_table(space->table);
    if (!forked->table)
        goto page_table_fork_failed;

    rw_spin_unlock_read_irq(&space->lock);

    return forked;

page_table_fork_failed:
area_clone_failed:
    while (forked->areas.size != 0) {
        kfree(array_list_remove_last(&forked->areas));
    }

    array_list_deinit(&forked->areas);

areas_list_initialization_failed:
    kfree(forked);
    rw_spin_unlock_read_irq(&space->lock);

    return NULL;
}

void vm_space_destroy(vm_space* space) {
    if (space->is_kernel_space)
        panic("Trying to destroy kernel vm");

    rw_spin_lock_write_irq(&space->lock);
    ref_release(&space->refc);

    if (space->refc.count != 0) {
        rw_spin_unlock_write_irq(&space->lock);
        return;
    }

    ARRAY_LIST_FOR_EACH(&space->areas, vm_area * area) kfree(area);

    array_list_deinit(&space->areas);
    arch_destroy_page_table(space->table);
    kfree(space);
}

static vm_page_mapping_result vm_space_map_page_unsafe(vm_space* space,
                                                       vaddr base,
                                                       vm_area_flags flags) {

    base = PAGE(base);
    vm_area* new = (vm_area*) kmalloc(sizeof(vm_area*));
    if (!new)
        return OUT_OF_MEMORY;

    if (!arch_map_page(space->table, base, flags)) {
        kfree(new);
        return OUT_OF_MEMORY;
    }

    new->base = base;
    new->length = PAGE_SIZE;
    new->flags = flags;

    if (!vm_space_insert_area_unsafe(space, new))
        return OUT_OF_MEMORY;

    return SUCCESS;
}

static vm_pages_mapping_result vm_space_map_pages_unsafe(vm_space* space,
                                                         vaddr base, u64 count,
                                                         vm_area_flags flags) {

    base = PAGE(base);
    vm_area temp = {.base = base, .length = PAGE_SIZE * count};

    if (vm_space_intersecting_area_unsafe(space, &temp))
        return (vm_pages_mapping_result){.mapped = 0, .status = ALREADY_MAPPED};

    u64 mapped = 0;
    vm_page_mapping_result page_status;

    for (; mapped < count; mapped++) {
        u64 page = base + PAGE_SIZE * mapped;
        page_status = vm_space_map_page_unsafe(space, page, flags);
        if (page_status != SUCCESS)
            break;
    }

    return (vm_pages_mapping_result){.mapped = mapped, .status = page_status};
}

vm_page_mapping_result vm_space_map_page(vm_space* space, vaddr base,
                                         vm_area_flags flags) {

    rw_spin_lock_write_irq(&space->lock);
    vm_page_mapping_result result =
        vm_space_map_pages_unsafe(space, base, 1, flags).status;
    rw_spin_unlock_write_irq(&space->lock);

    return result;
}

vm_pages_mapping_result vm_space_map_pages(vm_space* space, vaddr base,
                                           u64 count, vm_area_flags flags) {

    rw_spin_lock_write_irq(&space->lock);
    vm_pages_mapping_result result =
        vm_space_map_pages_unsafe(space, base, count, flags);
    rw_spin_unlock_write_irq(&space->lock);

    return result;
}

void* vm_space_get_page_view(vm_space* space, vaddr base) {
    base = PAGE(base);
    vm_area temp = {.base = base, .length = PAGE_SIZE};

    rw_spin_lock_read_irq(&space->lock);
    void* view = NULL;
    if (vm_space_surrounding_area_unsafe(space, &temp)) {
        view = arch_get_page_view(space->table, base);
    }
    rw_spin_unlock_read_irq(&space->lock);

    return view;
}

static bool vm_space_unmap_page_unsafe(vm_space* space, vaddr base) {
    vm_area temp = {.base = PAGE(base), .length = PAGE_SIZE};
    if (vm_space_cut_area_unsafe(space, &temp)) {
        arch_unmap_page(space->table, base);
        return true;
    }
    return false;
}

static bool vm_space_unmap_pages_unsafe(vm_space* space, vaddr base,
                                        u64 count) {
    bool result = false;
    base = PAGE(base);
    vm_area temp = {.base = base, .length = PAGE_SIZE * count};

    if (vm_space_surrounding_area_unsafe(space, &temp)) {
        for (u64 i = 0; i < count; i++) {
            vm_space_unmap_page_unsafe(space, base + i * PAGE_SIZE);
        }
        result = true;
    }

    return result;
}

bool vm_space_unmap_page(vm_space* space, vaddr base) {
    rw_spin_lock_write_irq(&space->lock);
    bool result = vm_space_unmap_pages_unsafe(space, base, 1);
    rw_spin_unlock_write_irq(&space->lock);

    return result;
}

bool vm_space_unmap_pages(vm_space* space, vaddr base, u64 count) {
    rw_spin_lock_write_irq(&space->lock);
    bool result = vm_space_unmap_pages_unsafe(space, base, count);
    rw_spin_unlock_write_irq(&space->lock);

    return result;
}

void vm_space_print(vm_space* space) {
    print("VM space");
    print(space->is_kernel_space ? "(kernel)" : "(user)");
    print(", page table: ");
    print_u64_hex((u64) space->table);

    if (!space->areas.size) {
        println(", no owned vm areas");
        return;
    }

    println(", owned vm areas: ");
    ARRAY_LIST_FOR_EACH(&space->areas, vm_area * area) {
        print("base: ");
        print_u64_hex(area->base);
        print(", len: ");
        print_u64(area->length);
        print("(");
        print_u64(area->length / PAGE_SIZE);
        print(" pgs), f: ");
        if (area->flags.present)
            print("P");
        if (area->flags.writable)
            print("W");
        if (area->flags.user_access_allowed)
            print("U");
        if (area->flags.executable)
            print("X");
        println(";");
    }
}