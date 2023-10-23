#ifndef SOS_PAGING_H
#define SOS_PAGING_H

#include "../../../multiboot.h"

#define PT_ENTRIES 512
#define KERNEL_P4_ENTRY 256
#define KERNEL_VMAPPED_RAM_P4_ENTRY 273

#define P1_OFFSET(a) (((a) >> 12) & 0x1FF)
#define P2_OFFSET(a) (((a) >> 21) & 0x1FF)
#define P3_OFFSET(a) (((a) >> 30) & 0x1FF)
#define P4_OFFSET(a) (((a) >> 39) & 0x1FF)

#define PRESENT_ATTR 1 << 0
#define RW_ATTR 1 << 1
#define SUPERVISOR_ATTR 1 << 2

#define MASK_FLAGS(addr) ((addr) & ~0xFFF)
#define NEXT_PT(entry) ((page_table*) P2V(MASK_FLAGS(entry)))
#define NEXT_PTE(entry, lvl, page)                                             \
    (NEXT_PT(entry)->entries[P##lvl##_OFFSET(page)])

typedef struct {
    u64 entries[PT_ENTRIES];
} page_table;

extern page_table kernel_p4_table;
page_table* current_p4_table = &kernel_p4_table;

#endif // SOS_PAGING_H
