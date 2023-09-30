#include "interrupt_handlers.h"
#include "../io.h"
#include "../util.h"
#include "../vga_print.h"
#include "idt.h"

#define DEFINE_SOFTWARE_INTERRUPT_HANDLER(i)                                   \
    __attribute__((__interrupt__)) void handle_interrupt_##i(void* frame) {    \
        UNUSED(frame);                                                         \
        handle_software_interrupt(i);                                          \
    }

#define DEFINE_HARDWARE_INTERRUPT_HANDLER(i)                                   \
    __attribute__((__interrupt__)) void handle_interrupt_##i(void* frame) {    \
        UNUSED(frame);                                                         \
        handle_hardware_interrupt(i);                                          \
    }

void* interrupt_handlers[48] = {
    handle_interrupt_0,  handle_interrupt_1,  handle_interrupt_2,
    handle_interrupt_3,  handle_interrupt_4,  handle_interrupt_5,
    handle_interrupt_6,  handle_interrupt_7,  handle_interrupt_8,
    handle_interrupt_9,  handle_interrupt_10, handle_interrupt_11,
    handle_interrupt_12, handle_interrupt_13, handle_interrupt_14,
    handle_interrupt_15, handle_interrupt_16, handle_interrupt_17,
    handle_interrupt_18, handle_interrupt_19, handle_interrupt_20,
    handle_interrupt_21, handle_interrupt_22, handle_interrupt_23,
    handle_interrupt_24, handle_interrupt_25, handle_interrupt_26,
    handle_interrupt_27, handle_interrupt_28, handle_interrupt_29,
    handle_interrupt_30, handle_interrupt_31, handle_interrupt_32,
    handle_interrupt_33, handle_interrupt_34, handle_interrupt_35,
    handle_interrupt_36, handle_interrupt_37, handle_interrupt_38,
    handle_interrupt_39, handle_interrupt_40, handle_interrupt_41,
    handle_interrupt_42, handle_interrupt_43, handle_interrupt_44,
    handle_interrupt_45, handle_interrupt_46, handle_interrupt_47
};

const u8 OCW2_EOI_OFFSET = 5;

__attribute__((no_caller_saved_registers)) void handle_interrupt(
    u8 interrupt_number) {
    static u32 count = 0;

    print("Received interrupt #");
    print_u32(interrupt_number);
    print(": ");
    print_u32(count++);
    print_char('\n');
}

__attribute__((no_caller_saved_registers)) void handle_hardware_interrupt(
    u8 interrupt_number) {

    handle_interrupt(interrupt_number);

    outb(MASTER_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
    io_wait();

    // slave PIC interrupt
    if (interrupt_number >= 40 && interrupt_number < 48) {
        outb(SLAVE_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
        io_wait();
    }
}

__attribute__((no_caller_saved_registers)) void handle_software_interrupt(
    u8 interrupt_number) {

    handle_interrupt(interrupt_number);
}

DEFINE_SOFTWARE_INTERRUPT_HANDLER(0)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(1)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(2)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(3)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(4)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(5)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(6)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(7)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(8)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(9)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(10)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(11)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(12)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(13)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(14)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(15)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(16)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(17)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(18)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(19)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(20)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(21)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(22)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(23)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(24)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(25)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(26)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(27)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(28)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(29)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(30)
DEFINE_SOFTWARE_INTERRUPT_HANDLER(31)

DEFINE_HARDWARE_INTERRUPT_HANDLER(32)
DEFINE_HARDWARE_INTERRUPT_HANDLER(33)
DEFINE_HARDWARE_INTERRUPT_HANDLER(34)
DEFINE_HARDWARE_INTERRUPT_HANDLER(35)
DEFINE_HARDWARE_INTERRUPT_HANDLER(36)
DEFINE_HARDWARE_INTERRUPT_HANDLER(37)
DEFINE_HARDWARE_INTERRUPT_HANDLER(38)
DEFINE_HARDWARE_INTERRUPT_HANDLER(39)
DEFINE_HARDWARE_INTERRUPT_HANDLER(40)
DEFINE_HARDWARE_INTERRUPT_HANDLER(41)
DEFINE_HARDWARE_INTERRUPT_HANDLER(42)
DEFINE_HARDWARE_INTERRUPT_HANDLER(43)
DEFINE_HARDWARE_INTERRUPT_HANDLER(44)
DEFINE_HARDWARE_INTERRUPT_HANDLER(45)
DEFINE_HARDWARE_INTERRUPT_HANDLER(46)
DEFINE_HARDWARE_INTERRUPT_HANDLER(47)