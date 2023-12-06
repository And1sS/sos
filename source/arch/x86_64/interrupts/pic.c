#include "pic.h"
#include "../cpu/io.h"

const u8 ICW1_ICW4_OFFSET = 0;
const u8 ICW1_SINGLE_MODE_OFFSET = 1;
const u8 ICW1_CALL_ADDRESS_INTERVAL_OFFSET = 2;
const u8 ICW1_LEVEL_TRIGGERED_MODE = 3;

const u8 ICW4_8086_MODE_OFFSET = 0;
const u8 ICW4_AUTO_EOI_OFFSET = 1;
const u8 ICW4_BUFFERED_MODE_OFFSET = 2;
const u8 ICW4_SPECIAL_FULLY_NESTED_MODE_OFFSET = 4;

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

void pic_init(void) {
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

const u8 OCW2_EOI_OFFSET = 5;

void pic_ack(u8 irq_num) {
    outb(MASTER_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
    io_wait();

    // slave PIC interrupt
    if (irq_num >= 40 && irq_num < 48) {
        outb(SLAVE_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
        io_wait();
    }
}