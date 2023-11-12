#include "pit.h"
#include "../../../lib/types.h"
#include "../io.h"

const u8 SELECT_COUNTER_OFFSET = 6;
const u8 READ_WRITE_MODE_OFFSET = 4;
const u8 MODE_OFFSET = 1;
const u8 BCD_OFFSET = 0;

typedef enum {
    LSB = 0b01,
    MSB = 0b10,
    LSB_THEN_MSB = 0b11
} timer_read_write_mode;

u8 gen_control_word(u8 counter_number, timer_read_write_mode read_write_mode,
                    u8 mode, bool use_bcd) {
    return ((counter_number & 0b11) << SELECT_COUNTER_OFFSET)
           | ((read_write_mode & 0b11) << READ_WRITE_MODE_OFFSET)
           | ((mode & 0b111) << MODE_OFFSET) | ((use_bcd & 1) << BCD_OFFSET);
}

const u16 COUNTER_0_ADDR = 0x40;
const u16 COUNTER_1_ADDR = 0x41;
const u16 COUNTER_2_ADDR = 0x42;
const u16 CONTROL_WORD_ADDR = 0x43;

// frequency = 1193182 Hz / FREQUENCY_DIVIDER
const u16 FREQUENCY_DIVIDER = (1 << 16) - 1;

void pit_init() {
    outb(CONTROL_WORD_ADDR, gen_control_word(0, LSB_THEN_MSB, 2, false));
    io_wait();

    outb(COUNTER_0_ADDR, FREQUENCY_DIVIDER & 0xFF);
    io_wait();
    outb(COUNTER_0_ADDR, (FREQUENCY_DIVIDER >> 8) & 0xFF);
    io_wait();
}