#ifndef IDT_H
#define IDT_H

#include "../../../lib/types.h"

extern const u16 MASTER_PIC_COMMAND_ADDR;
extern const u16 MASTER_PIC_DATA_ADDR;
extern const u16 SLAVE_PIC_COMMAND_ADDR;
extern const u16 SLAVE_PIC_DATA_ADDR;

void idt_init(void);

#endif // IDT_H
