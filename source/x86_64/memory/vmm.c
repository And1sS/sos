#include "../../memory/vmm.h"
#include "../../memory/pmm.h"
#include "../../spin_lock.h"

#define P1_OFFSET(a) (((a) >> 12) & 0x1FF)
#define P2_OFFSET(a) (((a) >> 21) & 0x1FF)
#define P3_OFFSET(a) (((a) >> 30) & 0x1FF)
#define P4_OFFSET(a) (((a) >> 39) & 0x1FF)

#define PT_ENTRIES 512
#define KERNEL_P4_ENTRY 256
#define KERNEL_VMAPPED_RAM_P4_ENTRY 273

#define PRESENT_ATTR 1 << 0
#define RW_ATTR 1 << 1
#define SUPERVISOR_ATTR 1 << 2

#define MASK_FLAGS(addr) (addr) & ~0xFFF
#define NEXT_PT(entry) ((page_table*) P2V(MASK_FLAGS(entry)))
#define NEXT_PTE(entry, lvl, page)                                             \
    (NEXT_PT(entry)->entries[P##lvl##_OFFSET(page)])

typedef struct {
    u64 entries[PT_ENTRIES];
} page_table;

extern page_table kernel_p4_table;

lock vmm_guard = 1;

page_table* current_p4_table = &kernel_p4_table;
u64 p3_allocated = 0, p2_allocated = 0, p1_allocated = 0;

u64 get_page(vaddr virtual_page) {
    if (!IS_CANONICAL(virtual_page)) {
        return NULL;
    }

    virtual_page = MASK_FLAGS(virtual_page);

    bool interrupts_enabled = spin_lock_irq_save(&vmm_guard);
    u64 p4_entry = current_p4_table->entries[P4_OFFSET(virtual_page)];
    if (!(p4_entry & PRESENT_ATTR)) {
        spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);
        return NULL;
    }

    u64 p3_entry = NEXT_PTE(p4_entry, 3, virtual_page);
    if (!(p3_entry & PRESENT_ATTR)) {
        spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);
        return NULL;
    }

    u64 p2_entry = NEXT_PTE(p3_entry, 2, virtual_page);
    if (!(p2_entry & PRESENT_ATTR)) {
        spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);
        return NULL;
    }

    u64 result = NEXT_PTE(p2_entry, 1, virtual_page);
    spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);

    return result;
}

paddr create_empty_table() {
    paddr frame = allocate_frame();
    u8* ptr = (u8*) P2V(frame);
    for (int i = 0; i < FRAME_SIZE; ++i) {
        ptr[i] = 0;
    }
    return frame;
}

#include "../../vga_print.h"
bool map_page(vaddr virtual_page, paddr physical_page, u16 flags) {
    // TODO: consider adding check if physical page is not virtual_address
    if (!IS_CANONICAL(virtual_page)) {
        return false;
    }

    flags &= 0xFFF;
    virtual_page = MASK_FLAGS(virtual_page);
    physical_page = MASK_FLAGS(physical_page);

    bool interrupts_enabled = spin_lock_irq_save(&vmm_guard);
    u64 p4_entry = current_p4_table->entries[P4_OFFSET(virtual_page)];
    if (!(p4_entry & PRESENT_ATTR)) {
        print("allocating p3: ");
        paddr frame = create_empty_table(); // allocating p3
        p3_allocated++;
        print_u64_hex(frame);
        println("");
        if (frame == NULL) {
            spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);
            return false;
        }

        p4_entry = frame | flags;
        current_p4_table->entries[P4_OFFSET(virtual_page)] = p4_entry;
    }

    u64 p3_entry = NEXT_PTE(p4_entry, 3, virtual_page);
    if (!(p3_entry & PRESENT_ATTR)) {
        paddr frame = create_empty_table(); // allocating p2
        p2_allocated++;
        if (frame == NULL) {
            spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);
            return false;
        }

        p3_entry = frame | flags;
        NEXT_PTE(p4_entry, 3, virtual_page) = p3_entry;
    }

    u64 p2_entry = NEXT_PTE(p3_entry, 2, virtual_page);
    if (!(p2_entry & PRESENT_ATTR)) {
        paddr frame = create_empty_table(); // allocating p1
        p1_allocated++;
        if (frame == NULL) {
            spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);
            return false;
        }

        p2_entry = frame | flags;
        NEXT_PTE(p3_entry, 2, virtual_page) = p2_entry;
    }

    NEXT_PTE(p2_entry, 1, virtual_page) = physical_page | flags;
    spin_unlock_irq_restore(&vmm_guard, interrupts_enabled);

    return true;
}
