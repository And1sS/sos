#ifndef IDT_H
#define IDT_H

#include "types.h"

#define PRESENT_OFFSET 7
#define DPL_OFFSET 5
#define SIZE_OFFSET 3
#define INTERRUPT_TYPE_OFFSET 0

typedef struct __attribute__((__aligned__(8), __packed__)) {
    u16 offset_0_15;
    u16 segment_selector;
    u8 reserved;
    u8 flags;
    u16 offset_16_31;
} interrupt_descriptor;

#define gen_interrupt_descriptor_flags(present, dpl, size, gate_type)          \
    ((((present) &1) << PRESENT_OFFSET) | (((dpl) &0b11) << DPL_OFFSET)        \
     | (((size) &1) << SIZE_OFFSET)                                            \
     | (((gate_type) &1) << INTERRUPT_TYPE_OFFSET) | 0b110)

#define gen_interrupt_descriptor(segment_selector_, offset, present, dpl,      \
                                 size, gate_type)                              \
    {                                                                          \
        .offset_0_15 = (offset) &0xFFFF,                                       \
        .segment_selector = (segment_selector_) &0xFFFF, .reserved = 0,        \
        .flags =                                                               \
            gen_interrupt_descriptor_flags(present, dpl, size, gate_type),     \
        .offset_16_31 = ((offset) >> 16) & 0xFFFF                              \
    }

typedef struct __attribute__((__packed__)) {
    const u16 limit;
    const interrupt_descriptor* data;
} idt_descriptor;

extern interrupt_descriptor idt_data[32];
extern const idt_descriptor idt;

extern void load_idt(const idt_descriptor* idt);
extern void handle_interrupt_asm();

void init_idt();
void handle_interrupt();

#endif // IDT_H
