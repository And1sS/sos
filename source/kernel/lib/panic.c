#include "panic.h"
#include "../arch/common/idle.h"
#include "../interrupts/irq.h"
#include "kprint.h"

_Noreturn void panic(string message) {
    local_irq_disable();
    println("");
    print("[Kernel panic]: ");
    println(message);
    halt();

    __builtin_unreachable();
}
