bits 32

%define KERNEL_STACK_SIZE 8192

%define KERNEL_START_VADDR 0xFFFF800000000000
%define KERNEL_VMAPPED_RAM_START_VADDR 0XFFFF888000000000
%define KERNEL_START_ADDRESS 0x100000

%define V2P(addr) ((addr) - KERNEL_START_VADDR) ; virtual address to physical

section .boot32_text

extern kernel_main
extern boot_print_string
extern boot_panic
extern boot_move_cursor_next_line
extern boot_clear_screen

extern set_up_page_tables
extern enable_paging

%define CODE_SEGMENT_SELECTOR 1 << 3

global _start
_start:
    mov esp, V2P(kernel_stack + KERNEL_STACK_SIZE)

    push ebx ; to preserve multiboot structure address loaded by grub
    push eax ; to preserve eax register which should hold multiboot magic

    call boot_clear_screen
    mov esi, KERNEL_STARTUP_MESSAGE_STR
    call boot_print_string
    call boot_move_cursor_next_line

    pop eax
    call check_multiboot

    call check_cpuid
    call check_long_mode

    call set_up_page_tables
    call enable_paging

    lgdt [V2P(GDT64.pointer)]
    jmp CODE_SEGMENT_SELECTOR:V2P(long_mode_start)


%define MULTIBOOT2_MAGIC 0x36D76289
check_multiboot:
    cmp eax, MULTIBOOT2_MAGIC
    jne .no_multiboot
    ret
.no_multiboot:
    mov esi, MULTIBOOT_ERROR_STR
    call boot_panic


%define ID_ATTR 1 << 21

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
    mov esi, CPUID_ERROR_STR
    call boot_panic


%define CPUID_LONG_MODE_ATTR 1 << 29
%define CPUID_MAXIMUM_ROUTINE 0x80000000
%define CPUID_VERSION_INFORMATION_ROUTINE 0x80000001

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
    mov esi, LONG_MODE_ERROR_STR
    call boot_panic


bits 64

section .boot64_text

extern kernel_main
extern remove_low_mem_identity_mapping
extern remap_kernel
extern remap_ram

long_mode_start:
    ; now rsp contains physical address, we need to adjust it to with virtual
    mov rax, KERNEL_START_VADDR
    add rsp, rax

    mov rax, qword .upper_memory
    jmp rax

.upper_memory:
    call remove_low_mem_identity_mapping
    call remap_kernel
    ; now we can call any C kernel function

    call remap_ram

    ; here stack contains argument of multiboot structure, which should be passed wia rdi register
    ; following System V AMD64 ABI calling convention
    pop rdi
    call kernel_main

    jmp $
    ret ; unreachable


section .boot32_data

KERNEL_STARTUP_MESSAGE_STR db 'Booting sos kernel in long mode...', 0
MULTIBOOT_ERROR_STR db 'Multiboot2 is not configured.', 0
CPUID_ERROR_STR db 'Cpuid instruction is not supported.', 0
LONG_MODE_ERROR_STR db 'Long mode is not supported.', 0

section .boot64_data

align 4096
kernel_stack:
    resb KERNEL_STACK_SIZE

section .boot64_rodata

%define CODE_ATTR 1 << 43
%define CONFORMING_ATTR 1 << 44
%define SEGMENT_PRESENT_ATTR 1 << 47
%define LONG_MODE_ATTR 1 << 53

; temporary gdt to get to long mode and resetup it later properly
GDT64:
    dq 0
.code:
    dq CODE_ATTR | CONFORMING_ATTR | SEGMENT_PRESENT_ATTR | LONG_MODE_ATTR
.pointer:
    dw $ - GDT64 - 1
    dq GDT64
