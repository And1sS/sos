#include "panic.h"
#include "../idle.h"
#include "../interrupts/irq.h"
#include "../vga_print.h"

void panic(string message) {
    local_irq_disable();
    println(message);
    halt();
}
