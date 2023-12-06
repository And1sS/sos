#include "idt.h"
#include "../cpu/gdt.h"
#include "../cpu/io.h"

typedef struct __attribute__((__aligned__(8), __packed__)) {
    u16 offset_0_15;
    u16 segment_selector;
    u8 ist;
    u8 flags;
    u16 offset_16_31;
    u32 offset_32_63;
    u32 reserved;
} interrupt_descriptor;

typedef struct __attribute__((__packed__)) {
    const u16 limit;
    const interrupt_descriptor* data;
} idt_descriptor;

const int PRESENT_OFFSET = 7;
const int DPL_OFFSET = 5;
const int INTERRUPT_TYPE_OFFSET = 0;

interrupt_descriptor idt_data[256];
const idt_descriptor idt = {.data = idt_data, .limit = sizeof(idt_data) - 1};

u8 gen_interrupt_descriptor_flags(bool is_present, u8 dpl, bool is_trap_gate) {

    return ((is_present & 1) << PRESENT_OFFSET) | ((dpl & 0b11) << DPL_OFFSET)
           | ((is_trap_gate & 1) << INTERRUPT_TYPE_OFFSET) | 0b1110;
}

interrupt_descriptor gen_interrupt_descriptor(u16 segment_selector, u64 offset,
                                              bool is_present, u8 dpl,
                                              bool is_trap_gate) {

    interrupt_descriptor result = {
        .offset_0_15 = offset & 0xFFFF,
        .segment_selector = segment_selector,
        .ist = 0,
        .flags = gen_interrupt_descriptor_flags(is_present, dpl, is_trap_gate),
        .offset_16_31 = (offset >> 16) & 0xFFFF,
        .offset_32_63 = (offset >> 32) & 0xFFFFFFFF,
        .reserved = 0};

    return result;
}

#define SET_ISR(i)                                                             \
    extern void isr_##i();                                                     \
    idt_data[i] = gen_interrupt_descriptor(KERNEL_CODE_SEGMENT_SELECTOR,       \
                                           (u64) isr_##i, true, 0, false);

#define SET_ESR(i)                                                             \
    extern void esr_##i();                                                     \
    idt_data[i] = gen_interrupt_descriptor(KERNEL_CODE_SEGMENT_SELECTOR,       \
                                           (u64) esr_##i, true, 0, false);

void idt_init(void) {
    SET_ESR(0)
    SET_ESR(1)
    SET_ESR(2)
    SET_ESR(3)
    SET_ESR(4)
    SET_ESR(5)
    SET_ESR(6)
    SET_ESR(7)
    SET_ESR(8)
    SET_ESR(9)
    SET_ESR(10)
    SET_ESR(11)
    SET_ESR(12)
    SET_ESR(13)
    SET_ESR(14)
    SET_ESR(15)
    SET_ESR(16)
    SET_ESR(17)
    SET_ESR(18)
    SET_ESR(19)
    SET_ESR(20)
    SET_ESR(21)
    SET_ESR(22)
    SET_ESR(23)
    SET_ESR(24)
    SET_ESR(25)
    SET_ESR(26)
    SET_ESR(27)
    SET_ESR(28)
    SET_ESR(29)
    SET_ESR(30)
    SET_ESR(31)
    SET_ISR(32)
    SET_ISR(33)
    SET_ISR(34)
    SET_ISR(35)
    SET_ISR(36)
    SET_ISR(37)
    SET_ISR(38)
    SET_ISR(39)
    SET_ISR(40)
    SET_ISR(41)
    SET_ISR(42)
    SET_ISR(43)
    SET_ISR(44)
    SET_ISR(45)
    SET_ISR(46)
    SET_ISR(47)
    SET_ISR(250)

    __asm__ volatile("lidt %0" : : "m"(idt));
}