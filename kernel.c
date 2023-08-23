#include "gdt.h"
#include "idt.h"
#include "types.h"
#include "vga_print.h"

void init();

_Noreturn void kernel_main() {
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

void init() {
    init_gdt();
    init_idt();
}