#include "types.h"
#include "vga_print.h"
#include "gdt.h"

void kernel_main() {
    load_gdt(&gdt);

    clear_screen();
    for (char i = '0'; i < '3'; i++) {
        for (char j = '0'; j <= '9'; j++) {
            print_char(i);
            print_char(j);
            print_char('\n');
        }
    }
}