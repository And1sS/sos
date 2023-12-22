#include "idt.h"
#include "../cpu/gdt.h"
#include "../cpu/io.h"
#include "../cpu/privilege_level.h"

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

u8 gen_interrupt_descriptor_flags(bool is_present, privilege_level dpl,
                                  bool is_trap_gate) {

    return ((is_present & 1) << PRESENT_OFFSET) | ((dpl & 0b11) << DPL_OFFSET)
           | ((is_trap_gate & 1) << INTERRUPT_TYPE_OFFSET) | 0b1110;
}

interrupt_descriptor gen_interrupt_descriptor(u16 segment_selector, u64 offset,
                                              bool is_present,
                                              privilege_level dpl,
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

const u8 ICW1_ICW4_OFFSET = 0;
const u8 ICW1_SINGLE_MODE_OFFSET = 1;
const u8 ICW1_CALL_ADDRESS_INTERVAL_OFFSET = 2;
const u8 ICW1_LEVEL_TRIGGERED_MODE = 3;

const u8 ICW4_8086_MODE_OFFSET = 0;
const u8 ICW4_AUTO_EOI_OFFSET = 1;
const u8 ICW4_BUFFERED_MODE_OFFSET = 2;
const u8 ICW4_SPECIAL_FULLY_NESTED_MODE_OFFSET = 4;

typedef enum {
    NON_BUFFERED = 0,
    SLAVE_BUFFERED_MODE = 0b10,
    MASTER_BUFFERED_MODE = 0b11
} pic_buffered_mode;

u8 gen_icw1(bool icw4_needed, bool in_single_mode, bit call_address_interval,
            bool in_level_triggered_mode) {

    return ((icw4_needed & 1) << ICW1_ICW4_OFFSET)
           | ((in_single_mode & 1) << ICW1_SINGLE_MODE_OFFSET)
           | ((call_address_interval & 1) << ICW1_CALL_ADDRESS_INTERVAL_OFFSET)
           | ((in_level_triggered_mode & 1) << ICW1_LEVEL_TRIGGERED_MODE)
           | (1 << 4);
}

u8 gen_icw2(u8 interrupt_offset) { return interrupt_offset & 0b11111000; }

// slave on irq 2
u8 gen_icw3_master() { return 1 << 2; }

u8 gen_icw3_slave() { return 2; }

u8 gen_icw4(bool in_auto_eoi_mode, pic_buffered_mode buffered_mode,
            bool in_special_fully_nested_mode) {

    return (1 << ICW4_8086_MODE_OFFSET)
           | ((in_auto_eoi_mode & 1) << ICW4_AUTO_EOI_OFFSET)
           | ((buffered_mode & 0b11) << ICW4_BUFFERED_MODE_OFFSET)
           | ((in_special_fully_nested_mode & 1)
              << ICW4_SPECIAL_FULLY_NESTED_MODE_OFFSET);
}

const u16 MASTER_PIC_COMMAND_ADDR = 0x20;
const u16 MASTER_PIC_DATA_ADDR = 0x21;
const u16 SLAVE_PIC_COMMAND_ADDR = 0xA0;
const u16 SLAVE_PIC_DATA_ADDR = 0xA1;

void init_pic(void) {
    u8 icw1 = gen_icw1(true, false, 0, false);
    outb(MASTER_PIC_COMMAND_ADDR, icw1);
    io_wait();
    outb(SLAVE_PIC_COMMAND_ADDR, icw1);
    io_wait();

    outb(MASTER_PIC_DATA_ADDR, gen_icw2(32));
    io_wait();
    outb(SLAVE_PIC_DATA_ADDR, gen_icw2(40));
    io_wait();

    outb(MASTER_PIC_DATA_ADDR, gen_icw3_master());
    io_wait();
    outb(SLAVE_PIC_DATA_ADDR, gen_icw3_slave());
    io_wait();

    u8 icw4 = gen_icw4(false, NON_BUFFERED, false);
    outb(MASTER_PIC_DATA_ADDR, icw4);
    io_wait();
    outb(SLAVE_PIC_DATA_ADDR, icw4);
    io_wait();

    // enable all interrupts
    u8 ocw1 = 0;
    outb(MASTER_PIC_DATA_ADDR, ocw1);
    io_wait();
    outb(SLAVE_PIC_DATA_ADDR, ocw1);
    io_wait();
}

#define SET_ISR(i)                                                             \
    extern void isr_##i();                                                     \
    idt_data[i] = gen_interrupt_descriptor(KERNEL_CODE_SEGMENT_SELECTOR,       \
                                           (u64) isr_##i, true, PL_3, false);

void idt_init(void) {
    SET_ISR(0)
    SET_ISR(1)
    SET_ISR(2)
    SET_ISR(3)
    SET_ISR(4)
    SET_ISR(5)
    SET_ISR(6)
    SET_ISR(7)
    SET_ISR(8)
    SET_ISR(9)
    SET_ISR(10)
    SET_ISR(11)
    SET_ISR(12)
    SET_ISR(13)
    SET_ISR(14)
    SET_ISR(15)
    SET_ISR(16)
    SET_ISR(17)
    SET_ISR(18)
    SET_ISR(19)
    SET_ISR(20)
    SET_ISR(21)
    SET_ISR(22)
    SET_ISR(23)
    SET_ISR(24)
    SET_ISR(25)
    SET_ISR(26)
    SET_ISR(27)
    SET_ISR(28)
    SET_ISR(29)
    SET_ISR(30)
    SET_ISR(31)
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
    SET_ISR(128) // syscall entry
    SET_ISR(250)

    __asm__ volatile("lidt %0" : : "m"(idt));

    init_pic();
}