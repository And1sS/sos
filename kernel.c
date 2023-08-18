#include "gdt.h"
#include "idt.h"
#include "types.h"
#include "vga_print.h"

_Noreturn void kernel_main() {
    load_gdt(&gdt);
    init_idt();
    load_idt(&idt);

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