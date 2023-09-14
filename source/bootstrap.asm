bits 32


section .text
; Multiboot2 header

MAGIC_NUMBER equ 0xE85250D6
ARCHITECTURE equ 0 ; i386 protected mode
HEADER_LENGTH equ 16
CHECKSUM equ -MAGIC_NUMBER - ARCHITECTURE - HEADER_LENGTH

align 8

dd MAGIC_NUMBER
dd ARCHITECTURE
dd HEADER_LENGTH
dd CHECKSUM

; empty tag

dw 0
dw 0
dd 8

; Multiboot2 header end


extern kernel_main
extern _start
_start:
    mov esp, kernel_stack + KERNEL_STACK_SIZE

    push eax ; to preserve eax register which should hold multiboot magic

    call clear_screen
    mov esi, KERNEL_STARTUP_MESSAGE_STR
    call print_string
    call move_cursor_next_line


    pop eax
    call check_multiboot

    call check_cpuid
    call check_long_mode

    call set_up_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start


MULTIBOOT2_MAGIC equ 0x36D76289

check_multiboot:
    cmp eax, MULTIBOOT2_MAGIC
    jne .no_multiboot
    ret
.no_multiboot:
    mov esi, MULTIBOOT_ERROR_STR
    call print_error


check_cpuid:
    ; Check if CPUID is supported by attempting to flip the ID bit (bit 21)
    ; in the FLAGS register. If we can flip it, CPUID is available.

    ; Copy FLAGS in to EAX via stack
    pushfd
    pop eax

    ; Copy to ECX as well for comparing later on
    mov ecx, eax

    ; Flip the ID bit
    xor eax, 1 << 21

    ; Copy EAX to FLAGS via the stack
    push eax
    popfd

    ; Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfd
    pop eax

    ; Restore FLAGS from the old version stored in ECX (i.e. flipping the
    ; ID bit back if it was ever flipped).
    push ecx
    popfd

    ; Compare EAX and ECX. If they are equal then that means the bit
    ; wasn't flipped, and CPUID isn't supported.
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov esi, CPUID_ERROR_STR
    call print_error


check_long_mode:
    ; test if extended processor info in available
    mov eax, 0x80000000    ; implicit argument for cpuid
    cpuid                  ; get highest supported argument
    cmp eax, 0x80000001    ; it needs to be at least 0x80000001
    jb .no_long_mode       ; if it's less, the CPU is too old for long mode

    ; use extended info to test if long mode is available
    mov eax, 0x80000001    ; argument for extended processor info
    cpuid                  ; returns various feature bits in ecx and edx
    test edx, 1 << 29      ; test if the LM-bit is set in the D-register
    jz .no_long_mode       ; If it's not set, there is no long mode
    ret
.no_long_mode:
    mov esi, LONG_MODE_ERROR_STR
    call print_error


set_up_page_tables:
    ; map first P4 entry to P3 table
    mov eax, p3_table
    or eax, 0b11 ; present + writable
    mov [p4_table], eax

    ; map first P3 entry to P2 table
    mov eax, p2_table
    or eax, 0b11 ; present + writable
    mov [p3_table], eax

    mov ecx, 0         ; counter variable

.map_p2_table:
    ; map ecx-th P2 entry to a huge page that starts at address 2MiB*ecx
    mov eax, 0x200000  ; 2MiB
    mul ecx            ; start address of ecx-th page
    or eax, 0b10000011 ; present + writable + huge
    mov [p2_table + ecx * 8], eax ; map ecx-th entry

    inc ecx            ; increase counter
    cmp ecx, 512       ; if counter == 512, the whole P2 table is mapped
    jne .map_p2_table  ; else map the next entry

    ret


enable_paging:
    ; load P4 to cr3 register (cpu uses this to access the P4 table)
    mov eax, p4_table
    mov cr3, eax

    ; enable PAE-flag in cr4 (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; set the long mode bit in the EFER MSR (model specific register)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging in the cr0 register
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret


; Screen printing routines start

VGA_MEMORY_ADDR         equ 0xB8000
WHITE_ON_BLACK_ATTR     equ 0x0F
WHITE_ON_RED_ATTR       equ 0x4F

CONSOLE_WIDTH           equ 80
CONSOLE_HEIGHT          equ 25


print_error:
    push esi

    mov esi, ERROR_PREFIX_STR
    mov dh, WHITE_ON_RED_ATTR
    call print_string_with_attr

    pop esi
    mov dh, WHITE_ON_RED_ATTR
    call print_string_with_attr

    hlt


print_string:
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

    call move_cursor_next_line

.print_next_char:
    jmp print_string_with_attr

.done:
    ret


clear_screen:
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
    call move_cursor_next_line

    cmp dword [cursor_row], CONSOLE_HEIGHT
    jne .clear_row

.done:
    mov dword [cursor_row], 0
    mov dword [cursor_col], 0
    ret


move_cursor_next_line:
    inc dword [cursor_row]
    mov dword [cursor_col], 0
    ret


section .data
    cursor_col dd 0
    cursor_row dd 0

; Screen printing routines end


section .bss

KERNEL_STACK_SIZE equ 8192

; temporary page tables mappings to get to long mode and resetup it later properly
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096

align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

section .rodata

; temporary gdt to get to long mode and resetup it later properly
gdt64:
    dq 0 ; reserved entry
.code: equ $ - gdt64 ; new
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; code segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

KERNEL_STARTUP_MESSAGE_STR db 'Booting sos kernel in long mode...', 0
ERROR_PREFIX_STR db 'Error: ', 0
MULTIBOOT_ERROR_STR db 'Multiboot2 is not configured.', 0
CPUID_ERROR_STR db 'Cpuid instruction is not supported.', 0
LONG_MODE_ERROR_STR db 'Long mode is not supported.', 0

bits 64
section .text

extern kernel_main

long_mode_start:
    jmp kernel_main
    jmp $