#ifndef VGA_PRINT_H
#define VGA_PRINT_H

#include "types.h"

typedef enum {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
} VGA_Color;

extern const u16 COLUMN_WIDTH;
extern const u16 ROW_NUMBER;

void init_console();

__attribute__((no_caller_saved_registers)) void print_u32(u32 x);
__attribute__((no_caller_saved_registers)) void print_u32_hex(u32 x);
__attribute__((no_caller_saved_registers)) void print_char(char ch);
__attribute__((no_caller_saved_registers)) void print(const char* str);
__attribute__((no_caller_saved_registers)) void println(const char* str);
__attribute__((no_caller_saved_registers)) void print_char_with_color(
    u16 row, u16 col, char ch, VGA_Color foreground, VGA_Color background);
__attribute__((no_caller_saved_registers)) void clear_screen(void);

#endif // VGA_PRINT_H