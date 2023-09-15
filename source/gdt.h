#ifndef GDT_H
#define GDT_H

#include "types.h"

extern u16 KERNEL_CODE_SEGMENT_SELECTOR;
extern u16 KERNEL_DATA_SEGMENT_SELECTOR;
extern u16 USER_CODE_SEGMENT_SELECTOR;
extern u16 USER_DATA_SEGMENT_SELECTOR;

void init_gdt(void);

#endif // GDT_H
