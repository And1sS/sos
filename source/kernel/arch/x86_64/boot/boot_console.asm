; Screen printing routines

bits 32

section .boot32_text

ERROR_PREFIX_STR db 'Error: ', 0

cursor_col dd 0
cursor_row dd 0

%define VGA_MEMORY_ADDR 0xB8000
%define WHITE_ON_BLACK_ATTR 0x0F
%define WHITE_ON_RED_ATTR 0x4F

%define CONSOLE_WIDTH 80
%define CONSOLE_HEIGHT 25

global boot_panic
boot_panic:
    push esi

    mov esi, ERROR_PREFIX_STR
    mov dh, WHITE_ON_RED_ATTR
    call print_string_with_attr

    pop esi
    mov dh, WHITE_ON_RED_ATTR
    call print_string_with_attr

    hlt


global boot_print_string
boot_print_string:
    mov dh, WHITE_ON_BLACK_ATTR
    call print_string_with_attr
    ret


print_string_with_attr:
    lodsb

    cmp al, 0
    je .done

    mov ah, dh

    mov dword ecx, [cursor_col]
    imul ecx, ecx, 2

    mov dword ebx, [cursor_row]
    imul ebx, ebx, 2 * CONSOLE_WIDTH

    mov word [VGA_MEMORY_ADDR + ebx + ecx], ax

    mov eax, [cursor_col]
    inc eax
    mov [cursor_col], eax
    cmp eax, CONSOLE_WIDTH
    jne .print_next_char

.proceed_on_new_line:
    cmp dword [cursor_row], CONSOLE_HEIGHT
    je .done

    call boot_move_cursor_next_line

.print_next_char:
    jmp print_string_with_attr

.done:
    ret


global boot_clear_screen
boot_clear_screen:
    mov dword [cursor_col], 0
    mov dword [cursor_row], 0

.clear_row:
    mov ah, WHITE_ON_BLACK_ATTR
    mov al, ' '

    mov dword ecx, [cursor_col]
    imul ecx, ecx, 2

    mov dword ebx, [cursor_row]
    imul ebx, ebx, 2 * CONSOLE_WIDTH

    mov word [VGA_MEMORY_ADDR + ebx + ecx], ax

    mov dword eax, [cursor_col]
    inc eax
    mov dword [cursor_col], eax
    cmp eax, CONSOLE_WIDTH
    jne .clear_row

.done_clearing_row:
    call boot_move_cursor_next_line

    cmp dword [cursor_row], CONSOLE_HEIGHT
    jne .clear_row

.done:
    mov dword [cursor_row], 0
    mov dword [cursor_col], 0
    ret


global boot_move_cursor_next_line
boot_move_cursor_next_line:
    inc dword [cursor_row]
    mov dword [cursor_col], 0
    ret
