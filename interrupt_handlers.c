#include "interrupt_handlers.h"
#include "util.h"
#include "vga_print.h"

#define DEFINE_INTERRUPT_HANDLER(i)                                            \
    __attribute__((__interrupt__)) void handle_interrupt_##i(void* frame) {    \
        UNUSED(frame);                                                         \
        handle_interrupt(i);                                                   \
    }

DEFINE_INTERRUPT_HANDLER(0)
DEFINE_INTERRUPT_HANDLER(1)
DEFINE_INTERRUPT_HANDLER(2)
DEFINE_INTERRUPT_HANDLER(3)
DEFINE_INTERRUPT_HANDLER(4)
DEFINE_INTERRUPT_HANDLER(5)
DEFINE_INTERRUPT_HANDLER(6)
DEFINE_INTERRUPT_HANDLER(7)
DEFINE_INTERRUPT_HANDLER(8)
DEFINE_INTERRUPT_HANDLER(9)
DEFINE_INTERRUPT_HANDLER(10)
DEFINE_INTERRUPT_HANDLER(11)
DEFINE_INTERRUPT_HANDLER(12)
DEFINE_INTERRUPT_HANDLER(13)
DEFINE_INTERRUPT_HANDLER(14)
DEFINE_INTERRUPT_HANDLER(15)
DEFINE_INTERRUPT_HANDLER(16)
DEFINE_INTERRUPT_HANDLER(17)
DEFINE_INTERRUPT_HANDLER(18)
DEFINE_INTERRUPT_HANDLER(19)
DEFINE_INTERRUPT_HANDLER(20)
DEFINE_INTERRUPT_HANDLER(21)
DEFINE_INTERRUPT_HANDLER(22)
DEFINE_INTERRUPT_HANDLER(23)
DEFINE_INTERRUPT_HANDLER(24)
DEFINE_INTERRUPT_HANDLER(25)
DEFINE_INTERRUPT_HANDLER(26)
DEFINE_INTERRUPT_HANDLER(27)
DEFINE_INTERRUPT_HANDLER(28)
DEFINE_INTERRUPT_HANDLER(29)
DEFINE_INTERRUPT_HANDLER(30)
DEFINE_INTERRUPT_HANDLER(31)

void* interrupt_handlers[32] = {
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
    handle_interrupt_30, handle_interrupt_31};

__attribute__((no_caller_saved_registers)) void handle_interrupt(
    u8 interrupt_number) {
    u8 i1 = interrupt_number / 10;
    u8 i2 = interrupt_number % 10;

    print("Received interrupt #");
    print_char('0' + i1);
    print_char('0' + i2);
    print_char('\n');
}