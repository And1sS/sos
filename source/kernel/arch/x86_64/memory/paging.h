#ifndef SOS_PAGING_H
#define SOS_PAGING_H

#include "../../../boot/multiboot.h"

#define PT_ENTRIES 512
#define KERNEL_P4_ENTRY 256
#define KERNEL_VMAPPED_RAM_P4_START_ENTRY 273
#define KERNEL_VMAPPED_RAM_P4_END_ENTRY 401 // exclusive

#define P1_OFFSET(a) (((a) >> 12) & 0x1FF)
#define P2_OFFSET(a) (((a) >> 21) & 0x1FF)
#define P3_OFFSET(a) (((a) >> 30) & 0x1FF)
#define P4_OFFSET(a) (((a) >> 39) & 0x1FF)

#define PRESENT_ATTR 1 << 0
#define WRITABLE_ATTR 1 << 1
#define SUPERVISOR_ATTR 1 << 2
#define HUGE_PAGE_ATTR 1 << 7

#define FLAGS_MASK 0xFFF
#define MASK_FLAGS(addr) ((addr) & ~FLAGS_MASK)
#define GET_FLAGS(entry) (entry & FLAGS_MASK)
#define NEXT_PT(entry) ((page_table*) P2V(MASK_FLAGS(entry)))
#define NEXT_PTE(entry, lvl, page)                                             \
    (NEXT_PT(entry)->entries[P##lvl##_OFFSET(page)])

typedef struct __attribute__((__packed__)) {
    u64 entries[PT_ENTRIES];
} page_table;

extern page_table kernel_p4_table;

#endif // SOS_PAGING_H
