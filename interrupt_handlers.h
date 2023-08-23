#ifndef INTERRUPT_HANDLERS_H
#define INTERRUPT_HANDLERS_H

#include "types.h"

__attribute__((no_caller_saved_registers)) void handle_interrupt(
    u8 interrupt_number);

__attribute__((__interrupt__)) void handle_interrupt_0(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_1(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_2(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_3(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_4(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_5(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_6(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_7(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_8(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_9(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_10(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_11(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_12(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_13(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_14(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_15(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_16(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_17(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_18(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_19(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_20(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_21(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_22(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_23(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_24(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_25(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_26(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_27(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_28(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_29(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_30(void* frame);
__attribute__((__interrupt__)) void handle_interrupt_31(void* frame);

extern void* interrupt_handlers[32];

#endif // INTERRUPT_HANDLERS_H
