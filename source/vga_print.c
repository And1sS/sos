#include "vga_print.h"
#include "spin_lock.h"

#define WITH_CONSOLE_LOCK(block)                                               \
    do {                                                                       \
        bool interrupts_enabled = spin_lock_irq_save(&console_lock);           \
        block;                                                                 \
        spin_unlock_irq_restore(&console_lock, interrupts_enabled);            \
    } while (0)

lock console_lock;

const u16 COLUMN_WIDTH = 80;
const u16 ROW_NUMBER = 25;

volatile u16* const TEXT_BUFFER = (u16*) 0x000B8000;

u16 cur_row = ROW_NUMBER - 1;
u16 cur_col = 0;
VGA_Color cur_foreground = WHITE;
VGA_Color cur_background = BLACK;

void init_console() { init_lock(&console_lock); }

__attribute__((no_caller_saved_registers)) void print_u32_unsafe(u32 x);
__attribute__((no_caller_saved_registers)) void print_u32_hex_unsafe(u32 x);
__attribute__((no_caller_saved_registers)) void print_char_unsafe(char ch);
__attribute__((no_caller_saved_registers)) void print_unsafe(const char* str);
__attribute__((no_caller_saved_registers)) void println_unsafe(const char* str);
__attribute__((no_caller_saved_registers)) void print_char_with_color_unsafe(
    u16 row, u16 col, char ch, VGA_Color foreground, VGA_Color background);
__attribute__((no_caller_saved_registers)) void clear_screen_unsafe(void);

__attribute__((no_caller_saved_registers)) void print_u32(u32 x) {
    WITH_CONSOLE_LOCK(print_u32_unsafe(x));
}

__attribute__((no_caller_saved_registers)) void print_u32_hex(u32 x) {
    WITH_CONSOLE_LOCK(print_u32_hex_unsafe(x));
}

__attribute__((no_caller_saved_registers)) void print_char(char ch) {
    WITH_CONSOLE_LOCK(print_char_unsafe(ch));
}

__attribute__((no_caller_saved_registers)) void print(const char* str) {
    WITH_CONSOLE_LOCK(print_unsafe(str));
}

__attribute__((no_caller_saved_registers)) void println(const char* str) {
    WITH_CONSOLE_LOCK(println_unsafe(str));
}

__attribute__((no_caller_saved_registers)) void print_char_with_color(
    u16 row, u16 col, char ch, VGA_Color foreground, VGA_Color background) {

    WITH_CONSOLE_LOCK(
        print_char_with_color_unsafe(row, col, ch, foreground, background));
}

__attribute__((no_caller_saved_registers)) void clear_screen(void) {
    WITH_CONSOLE_LOCK(clear_screen_unsafe());
}

__attribute__((no_caller_saved_registers)) void new_line(void);

__attribute__((no_caller_saved_registers)) void print_char_unsafe(char ch) {
    if (ch == '\n' || cur_col == COLUMN_WIDTH) {
        new_line();
    }

    if (ch != '\n') {
        print_char_with_color_unsafe(cur_row, cur_col++, ch, cur_foreground,
                                     cur_background);
    }
}

__attribute__((no_caller_saved_registers)) void print_unsafe(const char* str) {
    while (*str) {
        print_char_unsafe(*str);
        str++;
    }
}

__attribute__((no_caller_saved_registers)) void println_unsafe(
    const char* str) {
    print_unsafe(str);
    print_char_unsafe('\n');
}

__attribute__((no_caller_saved_registers)) void new_line(void) {
    for (u16 row = 0; row < ROW_NUMBER - 1; row++) {
        for (u16 col = 0; col < COLUMN_WIDTH; col++) {
            TEXT_BUFFER[row * COLUMN_WIDTH + col] =
                TEXT_BUFFER[(row + 1) * COLUMN_WIDTH + col];
        }
    }
    for (u16 col = 0; col < COLUMN_WIDTH; col++) {
        print_char_with_color_unsafe(ROW_NUMBER - 1, col, ' ', cur_background,
                                     cur_background);
    }
    cur_col = 0;
}

__attribute__((no_caller_saved_registers)) void print_char_with_color_unsafe(
    u16 row, u16 col, char ch, VGA_Color foreground, VGA_Color background) {
    TEXT_BUFFER[row * COLUMN_WIDTH + col] =
        (background << 12) | (foreground << 8) | ch;
}

__attribute__((no_caller_saved_registers)) void clear_screen_unsafe(void) {
    for (u16 row = 0; row < ROW_NUMBER; row++) {
        for (u16 col = 0; col < COLUMN_WIDTH; col++) {
            print_char_with_color_unsafe(row, col, ' ', WHITE, BLACK);
        }
    }
    cur_row = ROW_NUMBER - 1;
    cur_col = 0;
}

__attribute__((no_caller_saved_registers)) void print_u32_unsafe(u32 x) {
    if (x == 0) {
        print_char_unsafe('0');
        return;
    }

    u8 digits[10];

    i8 digit_count = 0;
    while (x > 0) {
        digits[digit_count++] = x % 10;
        x /= 10;
    }

    digit_count--;
    while (digit_count >= 0) {
        print_char_unsafe(digits[digit_count--] + '0');
    }
}

char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
__attribute__((no_caller_saved_registers)) void print_u32_hex_unsafe(u32 x) {
    char digits[8];
    for (u8 i = 0; i < 8; i++) {
        digits[i] = hex_chars[x & 0xF];
        x >>= 4;
    }
    print_char_unsafe('0');
    print_char_unsafe('x');
    for (u8 i = 0; i < 8; i++) {
        print_char_unsafe(digits[7 - i]);
    }
}