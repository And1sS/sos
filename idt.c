#include "idt.h"
#include "vga_print.h"

interrupt_descriptor idt_data[32];
const idt_descriptor idt = {
    .data = idt_data,
    .limit = sizeof(idt_data) - 1
};

void init_idt() {
    for (int i = 0; i < 32; i++) {
        interrupt_descriptor temp =
            gen_interrupt_descriptor(0x08, (int) handle_interrupt_asm, 1, 0, 1, 0);
        idt_data[i] = temp;
    }
}

void handle_interrupt() {
    println("Received interrupt");
}