#include "gdt.h"
#include "interrupts/idt.h"
#include "types.h"
#include "vga_print.h"
#include "timer.h"

void init(void);

_Noreturn void kernel_main(void) {
    init();

    clear_screen();
    for (char i = '0'; i < '2'; i++) {
        for (char j = '0'; j <= '9'; j++) {
            print_char(i);
            print_char(j);
            print_char('\n');
        }
    }

    while (true) {
    }
}

void init(void) {
    init_gdt();

    init_console();

    init_timer();
    init_idt();
}