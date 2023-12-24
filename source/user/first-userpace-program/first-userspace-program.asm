bits 64

org 0x1000

section .text

%macro make_syscall 1
    mov rax, %1
    int 0x80
%endmacro

_start:
    mov rdi, signal_handler
    make_syscall 1

start:
    call trigger_temp_syscall
    jmp start

trigger_temp_syscall:
    mov rbx, 0

.loop:
    inc rbx
    cmp rbx, 10000000 ; loop a bit to not flood with syscalls
    jne .loop

    mov rdi, HELLO_WORLD_STR ; syscall arg0
    make_syscall 0
    ret

signal_handler:
    mov rdi, HELLO_WORLD_2_STR
    make_syscall 0 ; print

    make_syscall 4 ; sigreturn
    ret

HELLO_WORLD_STR: db 'Hello world from user space!', 0
HELLO_WORLD_2_STR: db 'Hello world from signal handler!', 0

US_MAPPED equ 0x1300
US_UNMAPPED equ 0x7cccfffffff
KS_MAPPED equ 0xffffc88000000000
