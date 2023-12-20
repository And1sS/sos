#include "../../../lib/kprint.h"
#include "../../../scheduler/scheduler.h"
#include "../cpu/io.h"
#include "idt.h"

const u8 OCW2_EOI_OFFSET = 5;
volatile u64 ticks = 0;

struct cpu_context* handle_interrupt(u8 interrupt_number, u64 error_code,
                                     struct cpu_context* context) {

    if (interrupt_number == 32) {
        ticks++;
        return context_switch(context);
    } else if (interrupt_number == 250) {
        return context_switch(context);
    } else {
        print("isr #");
        print_u64(interrupt_number);
        print(", error code: ");
        print_u64_hex(error_code);
        println("");
    }

    return context;
}

struct cpu_context* handle_hardware_interrupt(u8 interrupt_number,
                                              struct cpu_context* context) {

    struct cpu_context* new_context =
        handle_interrupt(interrupt_number, 0, context);

    outb(MASTER_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
    io_wait();

    // slave PIC interrupt
    if (interrupt_number >= 40 && interrupt_number < 48) {
        outb(SLAVE_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
        io_wait();
    }

    return new_context;
}

struct cpu_context* handle_software_interrupt(u8 interrupt_number,
                                              u64 error_code,
                                              struct cpu_context* context) {

    return handle_interrupt(interrupt_number, error_code, context);
}