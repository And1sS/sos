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

mov esp, kernel_stack + KERNEL_STACK_SIZE
call kernel_main
jmp $

global load_gdt
load_gdt:
mov eax, dword [esp + 4]
lgdt [eax]

jmp 08h:temp
temp:

mov eax, 0x10
mov ds, eax
mov ss, eax

mov eax, 0
mov es, eax
mov fs, eax
mov gs, eax

ret

section .bss

KERNEL_STACK_SIZE equ 8192

align 4                                     
kernel_stack:
    resb KERNEL_STACK_SIZE
