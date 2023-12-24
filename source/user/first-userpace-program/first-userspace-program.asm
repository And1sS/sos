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
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, HELLO_WORLD_2_STR
    make_syscall 3

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    ret

HELLO_WORLD_STR: db 'Hello world from user space!', 0
HELLO_WORLD_2_STR: db 'Hello world from signal handler!', 0

US_MAPPED equ 0x1300
US_UNMAPPED equ 0x7cccfffffff
KS_MAPPED equ 0xffffc88000000000
