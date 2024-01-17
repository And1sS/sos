#include "panic.h"
#include "../idle.h"
#include "../interrupts/irq.h"
#include "kprint.h"

void panic(string message) {
    local_irq_disable();
    println("");
    print("PANIC: ");
    println(message);
    halt();
}
