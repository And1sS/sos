bits 32


section .multiboot.data
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

KERNEL_START_VADDR                  equ 0xFFFF800000000000
KERNEL_VMAPPED_RAM_START_VADDR      equ 0XFFFF888000000000
KERNEL_START_ADDRESS                equ 0x100000

%define V2P(addr) ((addr) - KERNEL_START_VADDR) ; virtual address to physical

section .multiboot.text

extern kernel_main
extern _start
_start:
    mov esp, V2P(kernel_stack + KERNEL_STACK_SIZE)

    push ebx ; to preserve multiboot structure address loaded by grub
    push eax ; to preserve eax register which should hold multiboot magic

    call clear_screen
    mov esi, V2P(KERNEL_STARTUP_MESSAGE_STR)
    call print_string
    call move_cursor_next_line


    pop eax
    call check_multiboot

    call check_cpuid
    call check_long_mode

    call set_up_page_tables
    call enable_paging

    lgdt [V2P(GDT64.pointer)]
    jmp CODE_SEGMENT_SELECTOR:V2P(long_mode_start)


MULTIBOOT2_MAGIC equ 0x36D76289

check_multiboot:
    cmp eax, MULTIBOOT2_MAGIC
    jne .no_multiboot
    ret
.no_multiboot:
    mov esi, V2P(MULTIBOOT_ERROR_STR)
    call print_error


ID_ATTR equ 1 << 21

; Check if CPUID is supported by attempting to flip the ID bit (bit 21)
; in the FLAGS register. If we can flip it, CPUID is available.
check_cpuid:
    pushfd
    pop eax

    mov ecx, eax
    xor eax, ID_ATTR
    push eax
    popfd

    pushfd
    pop eax
    push ecx
    popfd

    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov esi, V2P(CPUID_ERROR_STR)
    call print_error

CPUID_LONG_MODE_ATTR                equ 1 << 29
CPUID_MAXIMUM_ROUTINE               equ 0x80000000
CPUID_VERSION_INFORMATION_ROUTINE   equ 0x80000001

check_long_mode:
    ; test if extended processor info in available
    mov eax, CPUID_MAXIMUM_ROUTINE                      ; implicit argument for cpuid
    cpuid
    cmp eax, CPUID_VERSION_INFORMATION_ROUTINE
    jb .no_long_mode

    mov eax, CPUID_VERSION_INFORMATION_ROUTINE
    cpuid                  
    test edx, CPUID_LONG_MODE_ATTR                      ; implicit return data
    jz .no_long_mode       
    ret
.no_long_mode:
    mov esi, V2P(LONG_MODE_ERROR_STR)
    call print_error


WRITABLE_ATTR                           equ 1 << 0
PRESENT_ATTR                            equ 1 << 1
PAGE_SIZE                               equ 4096
PT_ENTRIES                              equ 512
PT_KERNEL_SPACE_START_IDX               equ 256 ; 0xFFFF800000000000
PT_KERNEL_SPACE_VMAPPED_RAM_START_IDX   equ 273 ; 0XFFFF888000000000

set_up_page_tables:
    mov eax, V2P(p3_table)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(p4_table)], eax

    mov [V2P(p4_table) + PT_KERNEL_SPACE_START_IDX * 8], eax
    mov [V2P(p4_table) + PT_KERNEL_SPACE_VMAPPED_RAM_START_IDX * 8], eax

    mov eax, V2P(p2_table)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(p3_table)], eax

    mov eax, V2P(p1_table)
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(p2_table)], eax

.identity_map_first_2mb:
    mov eax, PAGE_SIZE
    mul ebx
    or eax, PRESENT_ATTR | WRITABLE_ATTR
    mov [V2P(p1_table) + ebx * 8], eax

    inc ebx
    cmp ebx, PT_ENTRIES
    jne .identity_map_first_2mb

    ret


PAE_ATTR                equ 1 << 5
EFER_LONG_MODE_ATTR     equ 1 << 8
PAGING_ENABLED_ATTR     equ 1 << 31

enable_paging:
    mov eax, V2P(p4_table)
    mov cr3, eax

    mov eax, cr4
    or eax, PAE_ATTR
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, EFER_LONG_MODE_ATTR
    wrmsr

    mov eax, cr0
    or eax, PAGING_ENABLED_ATTR
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

    mov esi, V2P(ERROR_PREFIX_STR)
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

    mov dword ecx, [V2P(cursor_col)]
    imul ecx, ecx, 2

    mov dword ebx, [V2P(cursor_row)]
    imul ebx, ebx, 2 * CONSOLE_WIDTH

    mov word [VGA_MEMORY_ADDR + ebx + ecx], ax

    mov eax, [V2P(cursor_col)]
    inc eax
    mov [V2P(cursor_col)], eax
    cmp eax, CONSOLE_WIDTH
    jne .print_next_char

.proceed_on_new_line:
    cmp dword [V2P(cursor_row)], CONSOLE_HEIGHT
    je .done

    call move_cursor_next_line

.print_next_char:
    jmp print_string_with_attr

.done:
    ret


clear_screen:
    mov dword [V2P(cursor_col)], 0
    mov dword [V2P(cursor_row)], 0

.clear_row:
    mov ah, WHITE_ON_BLACK_ATTR
    mov al, ' '

    mov dword ecx, [V2P(cursor_col)]
    imul ecx, ecx, 2

    mov dword ebx, [V2P(cursor_row)]
    imul ebx, ebx, 2 * CONSOLE_WIDTH

    mov word [VGA_MEMORY_ADDR + ebx + ecx], ax

    mov dword eax, [V2P(cursor_col)]
    inc eax
    mov dword [V2P(cursor_col)], eax
    cmp eax, CONSOLE_WIDTH
    jne .clear_row

.done_clearing_row:
    call move_cursor_next_line

    cmp dword [V2P(cursor_row)], CONSOLE_HEIGHT
    jne .clear_row

.done:
    mov dword [V2P(cursor_row)], 0
    mov dword [V2P(cursor_col)], 0
    ret


move_cursor_next_line:
    inc dword [V2P(cursor_row)]
    mov dword [V2P(cursor_col)], 0
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
p1_table:
    resb 4096

align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

section .rodata

CODE_ATTR                   equ 1 << 43
CONFORMING_ATTR             equ 1 << 44
SEGMENT_PRESENT_ATTR        equ 1 << 47
LONG_MODE_ATTR              equ 1 << 53

CODE_SEGMENT_SELECTOR       equ 1 << 3
; temporary gdt to get to long mode and resetup it later properly
GDT64:
    dq 0 
.code:
    dq CODE_ATTR | CONFORMING_ATTR | SEGMENT_PRESENT_ATTR | LONG_MODE_ATTR
.pointer:
    dw $ - GDT64 - 1
    dq GDT64

KERNEL_STARTUP_MESSAGE_STR db 'Booting sos kernel in long mode...', 0
ERROR_PREFIX_STR db 'Error: ', 0
MULTIBOOT_ERROR_STR db 'Multiboot2 is not configured.', 0
CPUID_ERROR_STR db 'Cpuid instruction is not supported.', 0
LONG_MODE_ERROR_STR db 'Long mode is not supported.', 0

bits 64
section .text

extern kernel_main

long_mode_start:
    ; now rsp contains physical address, we need to adjust it to with virtual
    mov rax, KERNEL_START_VADDR
    add rsp, rax

    mov rax, qword .upper_memory
    jmp rax

.upper_memory:
    mov qword rax, 0
    mov [qword p4_table], rax

    mov rax, cr3
    mov cr3, rax

    ; here stack contains argument of multiboot structure, which should be passed wia rdi register
    ; following System V AMD64 ABI calling convention
    pop rdi
    call kernel_main

    jmp $