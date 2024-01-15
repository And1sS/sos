#include "../../../memory/vmm.h"
#include "../../../memory/pmm.h"
#include "paging.h"

page_table* current_p4_table = &kernel_p4_table;

u64 get_page(vaddr virtual_page) {
    if (!IS_CANONICAL(virtual_page)) {
        return NULL;
    }

    virtual_page = MASK_FLAGS(virtual_page);

    u64 p4_entry = current_p4_table->entries[P4_OFFSET(virtual_page)];
    if (!(p4_entry & PRESENT_ATTR)) {
        return NULL;
    }

    u64 p3_entry = NEXT_PTE(p4_entry, 3, virtual_page);
    if (!(p3_entry & PRESENT_ATTR)) {
        return NULL;
    }

    u64 p2_entry = NEXT_PTE(p3_entry, 2, virtual_page);
    if (!(p2_entry & PRESENT_ATTR)) {
        return NULL;
    }

    u64 result = NEXT_PTE(p2_entry, 1, virtual_page);

    return result;
}

bool map_page(vaddr virtual_page, paddr physical_page, u16 flags) {
    // TODO: consider adding check if physical page is not virtual_address
    if (!IS_CANONICAL(virtual_page)) {
        return false;
    }

    flags &= 0xFFF;
    virtual_page = MASK_FLAGS(virtual_page);
    physical_page = MASK_FLAGS(physical_page);

    u64 p4_entry = current_p4_table->entries[P4_OFFSET(virtual_page)];
    if (!(p4_entry & PRESENT_ATTR)) {
        paddr frame = allocate_zeroed_frame(); // allocating p3
        if (frame == NULL) {
            return false;
        }

        p4_entry = frame | flags;
        current_p4_table->entries[P4_OFFSET(virtual_page)] = p4_entry;
    }

    u64 p3_entry = NEXT_PTE(p4_entry, 3, virtual_page);
    if (!(p3_entry & PRESENT_ATTR)) {
        paddr frame = allocate_zeroed_frame(); // allocating p2
        if (frame == NULL) {
            return false;
        }

        p3_entry = frame | flags;
        NEXT_PTE(p4_entry, 3, virtual_page) = p3_entry;
    }

    u64 p2_entry = NEXT_PTE(p3_entry, 2, virtual_page);
    if (!(p2_entry & PRESENT_ATTR)) {
        paddr frame = allocate_zeroed_frame(); // allocating p1
        if (frame == NULL) {
            return false;
        }

        p2_entry = frame | flags;
        NEXT_PTE(p3_entry, 2, virtual_page) = p2_entry;
    }

    NEXT_PTE(p2_entry, 1, virtual_page) = physical_page | flags;
    return true;
}
