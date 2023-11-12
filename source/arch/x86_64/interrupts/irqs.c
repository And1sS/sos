#include "../../../scheduler/scheduler.h"
#include "../../../vga_print.h"
#include "../io.h"
#include "idt.h"

const u8 OCW2_EOI_OFFSET = 5;
volatile u64 ticks = 0;

u64 handle_interrupt(u8 interrupt_number, u64 rsp) {
    ticks++;

    if (interrupt_number == 32) {
        return (u64) context_switch((struct cpu_context*) rsp);
    } else {
        print_u64(interrupt_number);
        println("");
    }
    return rsp;
}

u64 handle_hardware_interrupt(u8 interrupt_number, u64 rsp) {
    outb(MASTER_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
    io_wait();

    // slave PIC interrupt
    if (interrupt_number >= 40 && interrupt_number < 48) {
        outb(SLAVE_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
        io_wait();
    }

    return handle_interrupt(interrupt_number, rsp);
}

u64 handle_software_interrupt(u8 interrupt_number, u64 error_code, u64 rsp) {
    print("error code: ");
    print_u64(error_code);
    println("");

    return handle_interrupt(interrupt_number, rsp);
}