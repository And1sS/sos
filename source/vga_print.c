#include "vga_print.h"

const u16 COLUMN_WIDTH = 80;
const u16 ROW_NUMBER = 25;

volatile u16* const TEXT_BUFFER = (u16*) 0x000B8000;

u16 cur_row = ROW_NUMBER - 1;
u16 cur_col = 0;
VGA_Color cur_foreground = WHITE;
VGA_Color cur_background = BLACK;

__attribute__((no_caller_saved_registers)) void new_line(void);

__attribute__((no_caller_saved_registers)) void print_char(char ch) {
    if (ch == '\n' || cur_col == COLUMN_WIDTH) {
        new_line();
    } else {
        print_char_with_color(cur_row, cur_col++, ch, cur_foreground,
                              cur_background);
    }
}

__attribute__((no_caller_saved_registers)) void print(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

__attribute__((no_caller_saved_registers)) void println(const char* str) {
    print(str);
    print_char('\n');
}

__attribute__((no_caller_saved_registers)) void new_line(void) {
    for (u16 row = 0; row < ROW_NUMBER - 1; row++) {
        for (u16 col = 0; col < COLUMN_WIDTH; col++) {
            TEXT_BUFFER[row * COLUMN_WIDTH + col] =
                TEXT_BUFFER[(row + 1) * COLUMN_WIDTH + col];
        }
    }
    for (u16 col = 0; col < COLUMN_WIDTH; col++) {
        print_char_with_color(ROW_NUMBER - 1, col, ' ', cur_background,
                              cur_background);
    }
    cur_col = 0;
}

__attribute__((no_caller_saved_registers)) void print_char_with_color(
    u16 row, u16 col, char ch, VGA_Color foreground, VGA_Color background) {
    TEXT_BUFFER[row * COLUMN_WIDTH + col] =
        (background << 12) | (foreground << 8) | ch;
}

__attribute__((no_caller_saved_registers)) void clear_screen(void) {
    for (u16 row = 0; row < ROW_NUMBER; row++) {
        for (u16 col = 0; col < COLUMN_WIDTH; col++) {
            print_char_with_color(row, col, ' ', WHITE, BLACK);
        }
    }
    cur_row = ROW_NUMBER - 1;
    cur_col = 0;
}

__attribute__((no_caller_saved_registers)) void print_u32(u32 x) {
    if (x == 0) {
        print_char('0');
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
        print_char(digits[digit_count--] + '0');
    }
}