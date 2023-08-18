#include "vga_print.h"

const u16 COLUMN_WIDTH = 80;
const u16 ROW_NUMBER = 25;

u16* const TEXT_BUFFER = (u16*) 0x000B8000;

u16 cur_row = ROW_NUMBER - 1;
u16 cur_col = 0;
VGA_Color cur_foreground = WHITE;
VGA_Color cur_background = BLACK;

void new_line();

void print_char(char ch) {
    if (ch == '\n' || cur_col == COLUMN_WIDTH) {
        new_line();
    } else {
        print_char_with_color(cur_row, cur_col++, ch, cur_foreground,
                              cur_background);
    }
}

void print(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

void println(const char* str) {
    print(str);
    print_char('\n');
}

void new_line() {
    for (u16 row = 0; row < ROW_NUMBER - 1; row++) {
        for (u16 col = 0; col < COLUMN_WIDTH; col++) {
            TEXT_BUFFER[row * COLUMN_WIDTH + col] = TEXT_BUFFER[(row + 1) * COLUMN_WIDTH + col];
        }
    }
    for (u16 col = 0; col < COLUMN_WIDTH; col++) {
        print_char_with_color(ROW_NUMBER - 1, col, ' ', cur_background, cur_background);
    }
    cur_col = 0;
}

void print_char_with_color(u16 row, u16 col, char ch, VGA_Color foreground, VGA_Color background) {
    TEXT_BUFFER[row * COLUMN_WIDTH + col] = (background << 12) | (foreground << 8) | ch;
}

void clear_screen() {
    for (u16 row = 0; row < ROW_NUMBER; row++) {
        for (u16 col = 0; col < COLUMN_WIDTH; col++) {
            print_char_with_color(row, col, ' ', WHITE, BLACK);
        }
    }
    cur_row = ROW_NUMBER - 1;
    cur_col = 0;
}