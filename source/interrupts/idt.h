#ifndef IDT_H
#define IDT_H

#include "../types.h"

extern const u16 MASTER_PIC_COMMAND_ADDR;
extern const u16 MASTER_PIC_DATA_ADDR;
extern const u16 SLAVE_PIC_COMMAND_ADDR;
extern const u16 SLAVE_PIC_DATA_ADDR;

void init_idt(void);

#endif // IDT_H
