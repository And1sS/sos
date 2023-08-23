#include "idt.h"
#include "interrupt_handlers.h"
#include "types.h"

typedef struct __attribute__((__aligned__(8), __packed__)) {
    u16 offset_0_15;
    u16 segment_selector;
    u8 reserved;
    u8 flags;
    u16 offset_16_31;
} interrupt_descriptor;

typedef struct __attribute__((__packed__)) {
    const u16 limit;
    const interrupt_descriptor* data;
} idt_descriptor;

const int PRESENT_OFFSET = 7;
const int DPL_OFFSET = 5;
const int SIZE_OFFSET = 3;
const int INTERRUPT_TYPE_OFFSET = 0;

interrupt_descriptor idt_data[32];
const idt_descriptor idt = {.data = idt_data, .limit = sizeof(idt_data) - 1};

extern void load_idt(const idt_descriptor* idt);

u8 gen_interrupt_descriptor_flags(bit present, u8 dpl, bit size,
                                  bit gate_type) {
    return ((present & 1) << PRESENT_OFFSET) | ((dpl & 0b11) << DPL_OFFSET)
           | ((size & 1) << SIZE_OFFSET)
           | ((gate_type & 1) << INTERRUPT_TYPE_OFFSET) | 0b110;
}

interrupt_descriptor gen_interrupt_descriptor(u16 segment_selector, u32 offset,
                                              bit present, u8 dpl, bit size,
                                              bit gate_type) {

    interrupt_descriptor result = {
        .offset_0_15 = offset & 0xFFFF,
        .segment_selector = segment_selector,
        .reserved = 0,
        .flags = gen_interrupt_descriptor_flags(present, dpl, size, gate_type),
        .offset_16_31 = (offset >> 16) & 0xFFFF};

    return result;
}

void init_idt(void) {
    for (int i = 0; i < 32; i++) {
        idt_data[i] = gen_interrupt_descriptor(
            0x08, (u32) interrupt_handlers[i], 1, 0, 1, 0);
    }

    __asm__("lidt %0" : : "m"(idt));
    __asm__("sti");
}